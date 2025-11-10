/**
 * @file boot_init.h
 * @brief Bootloader initialization functions
 * 
 * This module handles the bootloader entry point and early system initialization
 * before the main application starts. It includes reset reason handling, cache
 * setup, interrupt matrix configuration, and UART initialization for bootloader
 * communication.
 */

#ifndef BOOT_INIT_H
#define BOOT_INIT_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Bootloader entry point
 * 
 * This is the first function called after reset. It performs:
 * - Reset reason detection
 * - Vector table setup
 * - Cache configuration
 * - Memory initialization
 * - Interrupt matrix setup
 * - UART initialization for bootloader
 * - CPU frequency setup
 * - Flash initialization
 * 
 * @note This function never returns - it either calls app_main or enters an error loop
 */
void bootloader_entry(void) __attribute__((noreturn));

/**
 * @brief Initialize CPU and system clocks
 * 
 * Sets up CPU frequency and system clocks. Logs CPU frequency on completion.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_cpu_start(void);

/**
 * @brief Initialize system clock
 * 
 * Configures system clock sources and frequencies.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_clock_init(void);

/**
 * @brief Initialize peripherals
 * 
 * Initializes basic peripheral subsystems.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_peripheral_init(void);

/**
 * @brief Initialize cache
 * 
 * Configures instruction and data caches.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_cache_init(void);

/**
 * @brief Initialize watchdog
 * 
 * Disables watchdog during boot process.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_watchdog_init(void);

/**
 * @brief Initialize flash
 * 
 * Initializes flash memory subsystem and validates flash configuration.
 * 
 * @return ESP_OK on success
 */
esp_err_t boot_flash_init(void);

#ifdef __cplusplus
}
#endif

#endif // BOOT_INIT_H

