/**
 * @file heartbeat.c
 * @brief Heartbeat implementation
 * 
 * Original: sub_42010FDC (send_heartbeat)
 * 
 * Sends periodic heartbeat messages (function code 193) to keep connections alive.
 */

#include "heartbeat.h"
#include "../protocol/data_process.h"
#include "../protocol/function_codes.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "heartbeat";

#define HEARTBEAT_INTERVAL_MS    10000  // 10 seconds

static data_process_handle_t s_data_handle = NULL;
static TimerHandle_t s_heartbeat_timer = NULL;

/**
 * @brief Heartbeat timer callback
 */
static void heartbeat_timer_callback(TimerHandle_t xTimer)
{
    if (s_data_handle == NULL) {
        return;
    }

    // Send heartbeat (function code 193)
    esp_err_t ret = data_process_send(s_data_handle, PROTOCOL_FC_HEARTBEAT, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to send heartbeat: %d", ret);
    } else {
        ESP_LOGD(TAG, "Heartbeat sent");
    }
}

/**
 * @brief Initialize heartbeat mechanism
 * 
 * Original: sub_42010FDC initialization
 */
esp_err_t heartbeat_init(void)
{
    // Create heartbeat timer
    s_heartbeat_timer = xTimerCreate("heartbeat_timer",
                                     pdMS_TO_TICKS(HEARTBEAT_INTERVAL_MS),
                                     pdTRUE,  // Auto-reload
                                     NULL,
                                     heartbeat_timer_callback);
    
    if (s_heartbeat_timer == NULL) {
        ESP_LOGE(TAG, "Failed to create heartbeat timer");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Heartbeat initialized (interval: %d ms)", HEARTBEAT_INTERVAL_MS);
    return ESP_OK;
}

/**
 * @brief Start heartbeat
 */
esp_err_t heartbeat_start(data_process_handle_t data_handle)
{
    if (s_heartbeat_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    s_data_handle = data_handle;

    // Start timer
    if (xTimerStart(s_heartbeat_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to start heartbeat timer");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Heartbeat started");
    return ESP_OK;
}

/**
 * @brief Stop heartbeat
 */
esp_err_t heartbeat_stop(void)
{
    if (s_heartbeat_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xTimerStop(s_heartbeat_timer, 0) != pdPASS) {
        ESP_LOGE(TAG, "Failed to stop heartbeat timer");
        return ESP_FAIL;
    }

    s_data_handle = NULL;
    ESP_LOGI(TAG, "Heartbeat stopped");
    return ESP_OK;
}

