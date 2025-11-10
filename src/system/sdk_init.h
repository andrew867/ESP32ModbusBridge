/**
 * @file sdk_init.h
 * @brief ESP-IDF SDK initialization wrapper
 * 
 * This module provides wrapper functions for ESP-IDF SDK initialization.
 * It documents the initialization sequence and provides hooks for custom
 * initialization if needed.
 */

#ifndef SDK_INIT_H
#define SDK_INIT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ESP-IDF SDK components
 * 
 * This function should be called early in app_main() to ensure all
 * ESP-IDF components are properly initialized. It initializes:
 * - NVS (Non-Volatile Storage)
 * - Event loop
 * - Network interface
 * - Other system components
 * 
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t sdk_init(void);

/**
 * @brief Deinitialize ESP-IDF SDK components
 * 
 * Cleanup function for SDK components. Should be called before restart
 * or shutdown if needed.
 * 
 * @return ESP_OK on success
 */
esp_err_t sdk_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // SDK_INIT_H

