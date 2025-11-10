/**
 * @file sdk_init.c
 * @brief ESP-IDF SDK initialization implementation
 */

#include "sdk_init.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

static const char *TAG = "sdk_init";

/**
 * @brief Initialize ESP-IDF SDK components
 * 
 * Initializes core ESP-IDF components required by the application.
 * This should be called early in app_main().
 */
esp_err_t sdk_init(void)
{
    esp_err_t ret = ESP_OK;

    // Initialize NVS (Non-Volatile Storage)
    // This is required for parameter storage, WiFi credentials, etc.
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_LOGW(TAG, "NVS partition needs to be erased, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS initialized");

    // Initialize the default event loop
    // Required for WiFi, Ethernet, and other network events
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Event loop initialized");

    // Initialize network interface
    // Required for WiFi and Ethernet
    ret = esp_netif_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize network interface: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Network interface initialized");

    ESP_LOGI(TAG, "SDK initialization complete");
    return ESP_OK;
}

/**
 * @brief Deinitialize ESP-IDF SDK components
 * 
 * Cleanup function for SDK components.
 */
esp_err_t sdk_deinit(void)
{
    // Most ESP-IDF components don't require explicit deinitialization
    // This function is provided for completeness and future use
    
    ESP_LOGI(TAG, "SDK deinitialization complete");
    return ESP_OK;
}

