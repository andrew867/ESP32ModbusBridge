/**
 * @file uart_rx_task.h
 * @brief UART receive task
 * 
 * Original: sub_42013DE6 (uart_rx_task, priority 5)
 */

#ifndef UART_RX_TASK_H
#define UART_RX_TASK_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize UART RX task
 * 
 * Sets up UART for terminal service and starts the receive task.
 * 
 * @return ESP_OK on success
 */
esp_err_t uart_rx_task_init(void);

/**
 * @brief Set RX callback
 * 
 * @param callback Callback function to call when data is received
 */
void uart_rx_task_set_callback(void (*callback)(uint8_t *data, size_t len));

#ifdef __cplusplus
}
#endif

#endif // UART_RX_TASK_H

