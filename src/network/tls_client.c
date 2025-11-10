/**
 * @file tls_client.c
 * @brief TLS client implementation
 */

#include "tls_client.h"
#include "esp_log.h"

static const char *TAG = "tls_client";

esp_err_t tls_client_init(void)
{
    ESP_LOGI(TAG, "TLS client initialized");
    return ESP_OK;
}

