/**
 * @file tls_client.h
 * @brief TLS client wrapper
 */

#ifndef TLS_CLIENT_H
#define TLS_CLIENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t tls_client_init(void);

#ifdef __cplusplus
}
#endif

#endif // TLS_CLIENT_H

