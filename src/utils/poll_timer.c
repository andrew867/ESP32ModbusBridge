/**
 * @file poll_timer.c
 * @brief Poll timer implementation
 * 
 * Original: sub_42013BC0 (poll_timer_setup)
 * 
 * Provides periodic polling mechanism for RS485 communication.
 */

#include "poll_timer.h"
#include "../config/param_manager.h"
#include "../config/param_ids.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>

static const char *TAG = "poll_timer";

#define POLL_TIMER_DEFAULT_PERIOD_MS  1000  // 1 second default

static esp_timer_handle_t s_poll_timer = NULL;
static void (*s_poll_callback)(void) = NULL;

/**
 * @brief Poll timer callback
 */
static void poll_timer_callback(void *arg)
{
    if (s_poll_callback != NULL) {
        s_poll_callback();
    }
}

/**
 * @brief Initialize poll timer
 * 
 * Original: sub_42013BC0 (poll_timer_setup)
 */
esp_err_t poll_timer_init(void)
{
    // Get poll period from parameters (param ID 8 - query_period)
    int32_t poll_period_ms = POLL_TIMER_DEFAULT_PERIOD_MS;
    esp_err_t ret = param_get_int(PARAM_ID_8, &poll_period_ms);
    if (ret == ESP_ERR_NOT_FOUND || poll_period_ms <= 0) {
        poll_period_ms = POLL_TIMER_DEFAULT_PERIOD_MS;
    }

    // Create periodic timer
    const esp_timer_create_args_t timer_args = {
        .callback = poll_timer_callback,
        .name = "poll_timer",
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .skip_unhandled_events = false,
    };

    ret = esp_timer_create(&timer_args, &s_poll_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create poll timer: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Poll timer initialized (period: %d ms)", poll_period_ms);
    return ESP_OK;
}

/**
 * @brief Start poll timer
 */
esp_err_t poll_timer_start(void (*callback)(void), uint32_t period_ms)
{
    if (s_poll_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (callback == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_poll_callback = callback;

    // Stop timer if already running
    esp_timer_stop(s_poll_timer);

    // Start periodic timer
    esp_err_t ret = esp_timer_start_periodic(s_poll_timer, period_ms * 1000);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start poll timer: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "Poll timer started (period: %lu ms)", period_ms);
    return ESP_OK;
}

/**
 * @brief Stop poll timer
 */
esp_err_t poll_timer_stop(void)
{
    if (s_poll_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = esp_timer_stop(s_poll_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop poll timer: %d", ret);
        return ret;
    }

    s_poll_callback = NULL;
    ESP_LOGI(TAG, "Poll timer stopped");
    return ESP_OK;
}

/**
 * @brief Set poll period
 */
esp_err_t poll_timer_set_period(uint32_t period_ms)
{
    if (s_poll_timer == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    bool was_running = (esp_timer_is_active(s_poll_timer) == true);

    if (was_running) {
        esp_timer_stop(s_poll_timer);
    }

    if (was_running) {
        esp_err_t ret = esp_timer_start_periodic(s_poll_timer, period_ms * 1000);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restart poll timer: %d", ret);
            return ret;
        }
    }

    ESP_LOGI(TAG, "Poll timer period set to %lu ms", period_ms);
    return ESP_OK;
}

