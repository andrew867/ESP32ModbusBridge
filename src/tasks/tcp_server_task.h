/**
 * @file tcp_server_task.h
 * @brief TCP server task
 * 
 * Original: sub_42012878 (TCPServerApp_Init), sub_4201427A (tcp_server_task)
 */

#ifndef TCP_SERVER_TASK_H
#define TCP_SERVER_TASK_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize TCP server task
 * 
 * Sets up TCP server socket and starts listening for connections.
 * Supports multiple concurrent client connections with TLS support.
 * 
 * @return ESP_OK on success
 */
esp_err_t tcp_server_task_init(void);

/**
 * @brief Check if TCP server is running
 * 
 * @return true if server is running, false otherwise
 */
bool tcp_server_task_is_running(void);

/**
 * @brief Get number of connected clients
 * 
 * @return Number of connected clients
 */
int tcp_server_task_get_client_count(void);

#ifdef __cplusplus
}
#endif

#endif // TCP_SERVER_TASK_H

