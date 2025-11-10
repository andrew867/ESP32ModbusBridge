/**
 * @file param_manager.h
 * @brief Parameter management system
 * 
 * This module provides functions for managing device parameters stored in NVS.
 * Parameters can be strings or integers, and are identified by parameter IDs (0-15).
 * 
 * Original functions:
 * - sub_420107A4 -> param_set
 * - sub_42010952 -> param_get
 */

#ifndef PARAM_MANAGER_H
#define PARAM_MANAGER_H

#include "esp_err.h"
#include "param_ids.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameter type
 */
typedef enum {
    PARAM_TYPE_STRING = 0,  // String parameter
    PARAM_TYPE_INT = 1      // Integer parameter
} param_type_t;

/**
 * @brief Initialize parameter manager
 * 
 * Initializes NVS and loads all parameters from storage.
 * 
 * @return ESP_OK on success
 */
esp_err_t param_manager_init(void);

/**
 * @brief Set a string parameter
 * 
 * Original: sub_420107A4 (for string parameters)
 * Sets a string parameter value. The value is validated and stored in NVS.
 * 
 * @param id Parameter ID (0-15)
 * @param value String value to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t param_set_string(param_id_t id, const char *value);

/**
 * @brief Set an integer parameter
 * 
 * Sets an integer parameter value. The value is validated and stored in NVS.
 * 
 * @param id Parameter ID (0-15)
 * @param value Integer value to set
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t param_set_int(param_id_t id, int32_t value);

/**
 * @brief Get a string parameter
 * 
 * Original: sub_42010952 (for string parameters)
 * Retrieves a string parameter value from NVS or returns default if not set.
 * 
 * @param id Parameter ID (0-15)
 * @param value Buffer to store the value
 * @param max_len Maximum length of the buffer
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not set, error code otherwise
 */
esp_err_t param_get_string(param_id_t id, char *value, size_t max_len);

/**
 * @brief Get an integer parameter
 * 
 * Original: sub_42010952 (for integer parameters)
 * Retrieves an integer parameter value from NVS or returns default if not set.
 * 
 * @param id Parameter ID (0-15)
 * @param value Pointer to store the value
 * @return ESP_OK on success, ESP_ERR_NOT_FOUND if not set, error code otherwise
 */
esp_err_t param_get_int(param_id_t id, int32_t *value);

/**
 * @brief Get parameter type
 * 
 * Returns the type (string or int) for a given parameter ID.
 * 
 * @param id Parameter ID
 * @param type Pointer to store the type
 * @return ESP_OK on success
 */
esp_err_t param_get_type(param_id_t id, param_type_t *type);

/**
 * @brief Commit parameter changes
 * 
 * Commits all pending parameter changes to NVS.
 * 
 * @return ESP_OK on success
 */
esp_err_t param_commit(void);

/**
 * @brief Reset parameter to default value
 * 
 * Resets a parameter to its default value.
 * 
 * @param id Parameter ID
 * @return ESP_OK on success
 */
esp_err_t param_reset(param_id_t id);

/**
 * @brief Reset all parameters to defaults
 * 
 * Resets all parameters to their default values.
 * 
 * @return ESP_OK on success
 */
esp_err_t param_reset_all(void);

#ifdef __cplusplus
}
#endif

#endif // PARAM_MANAGER_H

