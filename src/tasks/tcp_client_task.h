/**
 * @file tcp_client_task.h
 * @brief TCP client task
 * 
 * Original: sub_4200D710 (TCPClientApp_Init), sub_42014D4A (tcp_client_task)
 */

#ifndef TCP_CLIENT_TASK_H
#define TCP_CLIENT_TASK_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize TCP client task
 * 
 * Sets up TCP client connection to server with TLS/SSL support.
 * Connects to "dongle_ssl.solarcloudsystem.com:4348" by default.
 * 
 * @return ESP_OK on success
 */
esp_err_t tcp_client_task_init(void);

/**
 * @brief Check if TCP client is connected
 * 
 * @return true if connected, false otherwise
 */
bool tcp_client_task_is_connected(void);

/**
 * @brief Send data through TCP client
 * 
 * @param data Data to send
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t tcp_client_task_send(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif // TCP_CLIENT_TASK_H

