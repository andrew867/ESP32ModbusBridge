/**
 * @file system_utils.c
 * @brief System utility functions implementation
 */

#include "system_utils.h"
#include "esp_log.h"
#include "esp_system.h"
#include "soc/rtc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "system_utils";

// Thread-local error code storage
static _Thread_local esp_err_t s_last_error = ESP_OK;

/**
 * @brief Reboot the system
 * 
 * Original: sub_4200070E
 * The original function called shutdown handlers before rebooting.
 * ESP-IDF's esp_restart() handles this automatically.
 */
void system_reboot(uint32_t delay_ms)
{
    if (delay_ms > 0) {
        ESP_LOGI(TAG, "Rebooting in %lu ms...", delay_ms);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
    
    ESP_LOGI(TAG, "Rebooting system...");
    esp_restart();
}

/**
 * @brief Get the reset reason
 * 
 * Converts ESP-IDF reset reason to our enum.
 */
system_reset_reason_t system_get_reset_reason(void)
{
    esp_reset_reason_t reason = esp_reset_reason();
    
    switch (reason) {
        case ESP_RST_UNKNOWN:
            return SYSTEM_RESET_REASON_UNKNOWN;
        case ESP_RST_POWERON:
            return SYSTEM_RESET_REASON_POWER_ON;
        case ESP_RST_EXT:
            return SYSTEM_RESET_REASON_EXT;
        case ESP_RST_SW:
            return SYSTEM_RESET_REASON_SOFTWARE;
        case ESP_RST_PANIC:
            return SYSTEM_RESET_REASON_CORE_PANIC;
        case ESP_RST_INT_WDT:
            return SYSTEM_RESET_REASON_INT_WDT;
        case ESP_RST_TASK_WDT:
            return SYSTEM_RESET_REASON_TASK_WDT;
        case ESP_RST_WDT:
            return SYSTEM_RESET_REASON_OTHER_WDT;
        case ESP_RST_DEEPSLEEP:
            return SYSTEM_RESET_REASON_DEEPSLEEP;
        case ESP_RST_BROWNOUT:
            return SYSTEM_RESET_REASON_BROWNOUT;
        case ESP_RST_SDIO:
            return SYSTEM_RESET_REASON_SDIO;
        default:
            return SYSTEM_RESET_REASON_OTHER;
    }
}

/**
 * @brief Get the last ESP error code pointer
 * 
 * Original: sub_42120952
 * Returns a pointer to thread-local error storage.
 */
esp_err_t* system_get_error_code_ptr(void)
{
    return &s_last_error;
}

/**
 * @brief Get the last ESP error code
 */
esp_err_t system_get_error_code(void)
{
    return s_last_error;
}

/**
 * @brief Set the error code
 */
void system_set_error_code(esp_err_t err)
{
    s_last_error = err;
}

/**
 * @brief Get FreeRTOS tick count
 * 
 * Original: sub_420027E8
 * Returns MEMORY[0x600B1004] which is the tick count register.
 * In ESP-IDF, we use xTaskGetTickCount().
 */
uint32_t system_get_tick_count(void)
{
    return xTaskGetTickCount();
}

/**
 * @brief Get tick count in milliseconds
 * 
 * Original: sub_4200D6FE
 * Returns (sub_420027E8() >> 7) which converts ticks to milliseconds
 * assuming 1000 Hz tick rate (tick >> 7 = tick / 128, but this seems wrong).
 * Actually, with 1000 Hz: tick_ms = tick / (1000 / 1000) = tick
 * But the original code does tick >> 7, which suggests a different calculation.
 * We'll use the proper ESP-IDF conversion.
 */
uint32_t system_get_tick_count_ms(void)
{
    return pdTICKS_TO_MS(xTaskGetTickCount());
}

/**
 * @brief Get system time in milliseconds
 * 
 * Original: sub_4211CFE0
 * Returns system time since boot in milliseconds.
 */
uint64_t system_get_time_ms(void)
{
    return (uint64_t)pdTICKS_TO_MS(xTaskGetTickCount());
}

/**
 * @brief Delay for specified milliseconds
 */
void system_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

