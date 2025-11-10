/**
 * @file tls_server.c
 * @brief TLS server implementation
 */

#include "tls_server.h"
#include "esp_log.h"

static const char *TAG = "tls_server";

esp_err_t tls_server_init(void)
{
    ESP_LOGI(TAG, "TLS server initialized");
    return ESP_OK;
}

