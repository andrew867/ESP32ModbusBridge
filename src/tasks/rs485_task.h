/**
 * @file rs485_task.h
 * @brief RS485/Modbus service task
 * 
 * Original: sub_420136F8 (rs485_service_task, priority 10)
 */

#ifndef RS485_TASK_H
#define RS485_TASK_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Frame callback function type
 */
typedef void (*rs485_frame_callback_t)(uint8_t *frame, size_t len);

/**
 * @brief Initialize RS485 task
 * 
 * Sets up UART for RS485 half-duplex communication and starts the service task.
 * 
 * @return ESP_OK on success
 */
esp_err_t rs485_task_init(void);

/**
 * @brief Set frame callback
 * 
 * @param callback Callback function to call when a valid frame is received
 */
void rs485_task_set_callback(rs485_frame_callback_t callback);

/**
 * @brief Send Modbus frame
 * 
 * @param frame Frame data (including address, function code, data, CRC)
 * @param len Frame length
 * @return ESP_OK on success
 */
esp_err_t rs485_task_send_frame(const uint8_t *frame, size_t len);

#ifdef __cplusplus
}
#endif

#endif // RS485_TASK_H

