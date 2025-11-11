/**
 * @file wifi_task.h
 * @brief WiFi management task
 * 
 * Original: sub_4200EB7C (wifi_task, priority 6), sub_4200EBBA (wifi_app_init)
 */

#ifndef WIFI_TASK_H
#define WIFI_TASK_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize WiFi application
 * 
 * Sets up WiFi in AP+STA mode and starts the WiFi management task.
 * 
 * @return ESP_OK on success
 */
esp_err_t wifi_task_init(void);

/**
 * @brief Check if WiFi is connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_task_is_connected(void);

/**
 * @brief Get current IP address
 * 
 * @param ip_str Buffer to store IP address string
 * @param len Buffer length
 * @return ESP_OK on success
 */
esp_err_t wifi_task_get_ip(char *ip_str, size_t len);

#ifdef __cplusplus
}
#endif

#endif // WIFI_TASK_H

