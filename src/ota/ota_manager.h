/**
 * @file ota_manager.h
 * @brief OTA update management
 * 
 * Original: sub_4200F4E6 (ota_update_start)
 */

#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize OTA manager
 * 
 * @return ESP_OK on success
 */
esp_err_t ota_manager_init(void);

/**
 * @brief Start OTA update from URL
 * 
 * @param url HTTPS URL to firmware image
 * @return ESP_OK on success
 */
esp_err_t ota_manager_start_update(const char *url);

#ifdef __cplusplus
}
#endif

#endif // OTA_MANAGER_H

