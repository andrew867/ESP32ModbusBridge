/**
 * @file system_utils.h
 * @brief System utility functions
 * 
 * This module provides system-level utility functions including:
 * - System reboot with shutdown handlers
 * - Reset reason handling
 * - Error code management
 * - Time and tick functions
 */

#ifndef SYSTEM_UTILS_H
#define SYSTEM_UTILS_H

#include <stdint.h>
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Reset reason codes
 * 
 * These correspond to ESP-IDF reset reasons
 */
typedef enum {
    SYSTEM_RESET_REASON_UNKNOWN = 0,
    SYSTEM_RESET_REASON_POWER_ON = 1,
    SYSTEM_RESET_REASON_EXT = 2,
    SYSTEM_RESET_REASON_SOFTWARE = 3,
    SYSTEM_RESET_REASON_CORE_PANIC = 4,
    SYSTEM_RESET_REASON_INT_WDT = 5,
    SYSTEM_RESET_REASON_TASK_WDT = 6,
    SYSTEM_RESET_REASON_OTHER_WDT = 7,
    SYSTEM_RESET_REASON_DEEPSLEEP = 8,
    SYSTEM_RESET_REASON_BROWNOUT = 9,
    SYSTEM_RESET_REASON_SDIO = 10,
    SYSTEM_RESET_REASON_EFUSE = 11,
    SYSTEM_RESET_REASON_USB = 12,
    SYSTEM_RESET_REASON_JTAG = 13,
    SYSTEM_RESET_REASON_CPU_LOCKUP = 14,
    SYSTEM_RESET_REASON_OTHER = 15
} system_reset_reason_t;

/**
 * @brief Reboot the system
 * 
 * Original: sub_4200070E
 * Reboots the system after calling all registered shutdown handlers.
 * This function never returns.
 * 
 * @param delay_ms Delay before reboot in milliseconds (0 for immediate)
 */
void system_reboot(uint32_t delay_ms) __attribute__((noreturn));

/**
 * @brief Get the reset reason
 * 
 * Returns the reason for the last system reset.
 * 
 * @return Reset reason code
 */
system_reset_reason_t system_get_reset_reason(void);

/**
 * @brief Get the last ESP error code
 * 
 * Original: sub_42120952
 * Returns a pointer to the thread-local error code storage.
 * 
 * @return Pointer to error code (can be written to set error)
 */
esp_err_t* system_get_error_code_ptr(void);

/**
 * @brief Get the last ESP error code
 * 
 * @return Last error code
 */
esp_err_t system_get_error_code(void);

/**
 * @brief Set the error code
 * 
 * @param err Error code to set
 */
void system_set_error_code(esp_err_t err);

/**
 * @brief Get FreeRTOS tick count
 * 
 * Original: sub_420027E8
 * Returns the current FreeRTOS tick count.
 * 
 * @return Tick count
 */
uint32_t system_get_tick_count(void);

/**
 * @brief Get tick count in milliseconds
 * 
 * Original: sub_4200D6FE
 * Returns the tick count converted to milliseconds.
 * 
 * @return Tick count in milliseconds
 */
uint32_t system_get_tick_count_ms(void);

/**
 * @brief Get system time in milliseconds
 * 
 * Original: sub_4211CFE0
 * Returns the system time in milliseconds since boot.
 * 
 * @return System time in milliseconds
 */
uint64_t system_get_time_ms(void);

/**
 * @brief Delay for specified milliseconds
 * 
 * @param ms Milliseconds to delay
 */
void system_delay_ms(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_UTILS_H

