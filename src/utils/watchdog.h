/**
 * @file watchdog.h
 * @brief Watchdog timer management
 * 
 * This module provides functions for managing the hardware watchdog timer.
 */

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize watchdog
 * 
 * Original: sub_420007E8
 * Initializes and configures the watchdog timer.
 * 
 * @return ESP_OK on success
 */
esp_err_t watchdog_init(void);

/**
 * @brief Feed the watchdog
 * 
 * Resets the watchdog timer to prevent system reset.
 * 
 * @return ESP_OK on success
 */
esp_err_t watchdog_feed(void);

/**
 * @brief Disable watchdog
 * 
 * Disables the watchdog timer (use with caution).
 * 
 * @return ESP_OK on success
 */
esp_err_t watchdog_disable(void);

#ifdef __cplusplus
}
#endif

#endif // WATCHDOG_H

