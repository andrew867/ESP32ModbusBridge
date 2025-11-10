/**
 * @file factory_test.h
 * @brief Factory test functionality
 * 
 * Original: sub_4200F518 (factory_test)
 */

#ifndef FACTORY_TEST_H
#define FACTORY_TEST_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize factory test
 * 
 * @return ESP_OK on success
 */
esp_err_t factory_test_init(void);

/**
 * @brief Enable factory test mode
 * 
 * @return ESP_OK on success
 */
esp_err_t factory_test_enable(void);

/**
 * @brief Disable factory test mode
 * 
 * @return ESP_OK on success
 */
esp_err_t factory_test_disable(void);

/**
 * @brief Check if factory test mode is enabled
 * 
 * @return true if factory test mode is enabled, false otherwise
 */
bool factory_test_is_enabled(void);

#ifdef __cplusplus
}
#endif

#endif // FACTORY_TEST_H

