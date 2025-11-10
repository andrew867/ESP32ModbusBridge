/**
 * @file tcp_client.h
 * @brief TCP client implementation
 */

#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tcp_client_init(void);

#ifdef __cplusplus
}
#endif

#endif // TCP_CLIENT_H

