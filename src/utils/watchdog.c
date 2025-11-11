/**
 * @file watchdog.c
 * @brief Watchdog timer management implementation
 */

#include "watchdog.h"
#include "esp_log.h"
#include "esp_task_wdt.h"

static const char *TAG = "watchdog";
static bool s_watchdog_initialized = false;

/**
 * @brief Initialize watchdog
 * 
 * Original: sub_420007E8
 * The original code disabled the watchdog during boot:
 * - wdt_hal_write_protect_disable()
 * - wdt_hal_disable()
 * - wdt_hal_write_protect_enable()
 * 
 * In ESP-IDF, we use the task watchdog which is enabled by default.
 */
esp_err_t watchdog_init(void)
{
    if (s_watchdog_initialized) {
        return ESP_OK;
    }

    // ESP-IDF task watchdog is enabled by default
    // In ESP-IDF v5.5, esp_task_wdt_init takes a config struct
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000,  // 30 second timeout
        .idle_core_mask = 0,  // Watch all cores
        .trigger_panic = true // Panic on timeout
    };
    
    esp_err_t ret = esp_task_wdt_init(&wdt_config);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize task watchdog: %s", esp_err_to_name(ret));
        return ret;
    }

    s_watchdog_initialized = true;
    ESP_LOGI(TAG, "Watchdog initialized");
    return ESP_OK;
}

/**
 * @brief Feed the watchdog
 */
esp_err_t watchdog_feed(void)
{
    if (!s_watchdog_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return esp_task_wdt_reset();
}

/**
 * @brief Disable watchdog
 * 
 * WARNING: Disabling the watchdog can cause the system to hang
 * if a task stops responding. Use with extreme caution.
 */
esp_err_t watchdog_disable(void)
{
    if (!s_watchdog_initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    // ESP-IDF doesn't provide a direct way to disable the task watchdog
    // This would require low-level HAL access
    ESP_LOGW(TAG, "Watchdog disable not fully supported in ESP-IDF");
    return ESP_ERR_NOT_SUPPORTED;
}

