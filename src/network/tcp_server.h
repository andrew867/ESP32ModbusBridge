/**
 * @file tcp_server.h
 * @brief TCP server implementation
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tcp_server_init(void);

#ifdef __cplusplus
}
#endif

#endif // TCP_SERVER_H

