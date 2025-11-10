/**
 * @file wifi_task.c
 * @brief WiFi task implementation
 * 
 * Original: sub_4200EBBA (wifi_app_init), sub_4200EB7C (wifi_task, priority 6)
 * 
 * Handles:
 * - WiFi initialization in AP+STA mode
 * - Event handling for connection/disconnection
 * - Static IP configuration
 * - Connection management
 */

#include "wifi_task.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <stdbool.h>

static const char *TAG = "wifi_task";

#define WIFI_AP_SSID_DEFAULT      "LuxPower"
#define WIFI_AP_PASSWORD_DEFAULT  ""
#define WIFI_AP_CHANNEL           1
#define WIFI_AP_MAX_CONNECTIONS   4
#define WIFI_STA_CONNECT_TIMEOUT  30000  // 30 seconds

static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;
static EventGroupHandle_t s_wifi_event_group = NULL;
static bool s_wifi_connected = false;
static bool s_wifi_ap_started = false;

#define WIFI_CONNECTED_BIT    BIT0
#define WIFI_FAIL_BIT         BIT1

/**
 * @brief WiFi event handler
 * 
 * Original: off_4200E0DE (WIFIApp_EventHandler)
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WiFi AP started");
                s_wifi_ap_started = true;
                break;

            case WIFI_EVENT_AP_STOP:
                ESP_LOGI(TAG, "WiFi AP stopped");
                s_wifi_ap_started = false;
                break;

            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi STA started");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WiFi STA connected");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WiFi STA disconnected");
                s_wifi_connected = false;
                xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                // Attempt to reconnect
                esp_wifi_connect();
                break;

            default:
                break;
        }
    } else if (event_base == IP_EVENT) {
        if (event_id == IP_EVENT_STA_GOT_IP) {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
            ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            s_wifi_connected = true;
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

/**
 * @brief Configure AP mode
 * 
 * Original: sub_4200E4AA (ApConfig)
 */
static esp_err_t wifi_configure_ap(void)
{
    char ssid[64];
    char password[64];

    // Get SSID and password from parameters
    esp_err_t ret = param_get_string(PARAM_ID_2, ssid, sizeof(ssid));
    if (ret == ESP_ERR_NOT_FOUND) {
        strncpy(ssid, WIFI_AP_SSID_DEFAULT, sizeof(ssid) - 1);
    }

    ret = param_get_string(PARAM_ID_3, password, sizeof(password));
    if (ret == ESP_ERR_NOT_FOUND) {
        strncpy(password, WIFI_AP_PASSWORD_DEFAULT, sizeof(password) - 1);
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ssid),
            .channel = WIFI_AP_CHANNEL,
            .password = "",
            .max_connection = WIFI_AP_MAX_CONNECTIONS,
            .authmode = (strlen(password) > 0) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN,
        },
    };

    strncpy((char*)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    if (strlen(password) > 0) {
        strncpy((char*)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_LOGI(TAG, "AP configured: SSID=%s", ssid);
    return ESP_OK;
}

/**
 * @brief Configure STA mode
 * 
 * Original: sub_4200E566 (StaConfig)
 */
static esp_err_t wifi_configure_sta(void)
{
    char ssid[64];
    char password[64];

    // Get SSID and password from parameters
    esp_err_t ret = param_get_string(PARAM_ID_2, ssid, sizeof(ssid));
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "WiFi SSID not configured");
        return ESP_ERR_NOT_FOUND;
    }

    ret = param_get_string(PARAM_ID_3, password, sizeof(password));
    if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "WiFi password not configured");
        return ESP_ERR_NOT_FOUND;
    }

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_LOGI(TAG, "STA configured: SSID=%s", ssid);
    return ESP_OK;
}

/**
 * @brief Set static IP configuration
 * 
 * Original: sub_4200E8A6 (wifi_set_static_ip)
 */
static esp_err_t wifi_set_static_ip(void)
{
    int32_t ip_config = 0;
    esp_err_t ret = param_get_int(PARAM_ID_14, &ip_config);
    if (ret == ESP_ERR_NOT_FOUND) {
        ip_config = 0; // Default to DHCP
    }

    if (ip_config == 0) {
        // Use DHCP
        ESP_LOGI(TAG, "Using DHCP for IP configuration");
        return ESP_OK;
    }

    // Configure static IP
    esp_netif_ip_info_t ip_info;
    // Default static IP: 192.168.4.1
    IP4_ADDR(&ip_info.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip_info.netmask, 255, 255, 255, 0);

    ret = esp_netif_dhcpc_stop(s_sta_netif);
    if (ret != ESP_OK && ret != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STOPPED) {
        ESP_LOGE(TAG, "Failed to stop DHCP client: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_netif_set_ip_info(s_sta_netif, &ip_info);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set static IP: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Static IP configured: " IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

/**
 * @brief WiFi task
 * 
 * Original: sub_4200EB7C (wifi_task, priority 6)
 * Monitors WiFi connection status and attempts reconnection if needed.
 */
static void wifi_task(void *pvParameters)
{
    while (1) {
        // Wait for connection or timeout
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                               WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                               pdFALSE,
                                               pdFALSE,
                                               pdMS_TO_TICKS(WIFI_STA_CONNECT_TIMEOUT));

        if (bits & WIFI_CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi connected");
            vTaskDelay(pdMS_TO_TICKS(3000));
        } else if (bits & WIFI_FAIL_BIT) {
            ESP_LOGI(TAG, "WiFi connection failed, retrying...");
            esp_wifi_connect();
            vTaskDelay(pdMS_TO_TICKS(3000));
        } else {
            // Timeout - check connection status
            if (!s_wifi_connected) {
                ESP_LOGI(TAG, "WiFi not connected, attempting connection...");
                esp_wifi_connect();
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
}

/**
 * @brief Initialize WiFi application
 * 
 * Original: sub_4200EBBA (WIFIApp_Init)
 */
esp_err_t wifi_task_init(void)
{
    // Create event group
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    // Initialize network interface
    s_sta_netif = esp_netif_create_default_wifi_sta();
    s_ap_netif = esp_netif_create_default_wifi_ap();

    // Initialize WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    // Set WiFi mode to AP+STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure AP
    wifi_configure_ap();

    // Configure STA
    wifi_configure_sta();

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Set static IP if configured
    wifi_set_static_ip();

    // Create WiFi task (priority 6)
    xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 6, NULL);

    ESP_LOGI(TAG, "WiFi application initialized");
    return ESP_OK;
}

/**
 * @brief Check if WiFi is connected
 */
bool wifi_task_is_connected(void)
{
    return s_wifi_connected;
}

/**
 * @brief Get current IP address
 */
esp_err_t wifi_task_get_ip(char *ip_str, size_t len)
{
    if (ip_str == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_wifi_connected || s_sta_netif == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_netif_ip_info_t ip_info;
    esp_err_t ret = esp_netif_get_ip_info(s_sta_netif, &ip_info);
    if (ret != ESP_OK) {
        return ret;
    }

    snprintf(ip_str, len, IPSTR, IP2STR(&ip_info.ip));
    return ESP_OK;
}

