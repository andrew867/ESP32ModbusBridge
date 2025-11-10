/**
 * @file tcp_client.c
 * @brief TCP client implementation
 */

#include "tcp_client.h"
#include "esp_log.h"

static const char *TAG = "tcp_client";

esp_err_t tcp_client_init(void)
{
    ESP_LOGI(TAG, "TCP client initialized");
    return ESP_OK;
}

