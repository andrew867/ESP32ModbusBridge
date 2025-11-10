/**
 * @file ble_task.h
 * @brief BLE application task
 * 
 * Original: sub_4201000E (ble_app_task, priority 10), sub_4200F298 (ble_data_receive)
 */

#ifndef BLE_TASK_H
#define BLE_TASK_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize BLE task
 * 
 * Sets up BLE GATT server with characteristics for data communication.
 * Starts BLE advertising.
 * 
 * @return ESP_OK on success
 */
esp_err_t ble_task_init(void);

#ifdef __cplusplus
}
#endif

#endif // BLE_TASK_H

