/**
 * @file tcp_server.c
 * @brief TCP server implementation
 */

#include "tcp_server.h"
#include "esp_log.h"

static const char *TAG = "tcp_server";

esp_err_t tcp_server_init(void)
{
    ESP_LOGI(TAG, "TCP server initialized");
    return ESP_OK;
}

