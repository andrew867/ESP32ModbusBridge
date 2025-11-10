/**
 * @file heartbeat.h
 * @brief Heartbeat mechanism
 * 
 * Original: sub_42010FDC (send_heartbeat)
 */

#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "esp_err.h"
#include "../protocol/data_process.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize heartbeat mechanism
 * 
 * @return ESP_OK on success
 */
esp_err_t heartbeat_init(void);

/**
 * @brief Start heartbeat with data process handle
 * 
 * @param data_handle Data process handle to send heartbeat through
 * @return ESP_OK on success
 */
esp_err_t heartbeat_start(data_process_handle_t data_handle);

/**
 * @brief Stop heartbeat
 * 
 * @return ESP_OK on success
 */
esp_err_t heartbeat_stop(void);

#ifdef __cplusplus
}
#endif

#endif // HEARTBEAT_H

