/**
 * @file tls_server.h
 * @brief TLS server wrapper
 * 
 * Original: sub_4201427A (tls_server, priority 10)
 */

#ifndef TLS_SERVER_H
#define TLS_SERVER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tls_server_init(void);

#ifdef __cplusplus
}
#endif

#endif // TLS_SERVER_H

