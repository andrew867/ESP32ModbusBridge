/**
 * @file ringbuffer.c
 * @brief Ring buffer management implementation
 */

#include "ringbuffer.h"
#include "esp_log.h"
#include "freertos/ringbuf.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "ringbuffer";

/**
 * @brief Create a ring buffer for UART data
 */
esp_err_t ringbuffer_create_uart(size_t size, RingbufHandle_t *handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Create a byte buffer ring buffer
    *handle = xRingbufferCreate(size, RINGBUF_TYPE_BYTEBUF);
    if (*handle == NULL) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Ring buffer created, size: %zu bytes", size);
    return ESP_OK;
}

/**
 * @brief Send data to ring buffer
 */
esp_err_t ringbuffer_send(RingbufHandle_t handle, const void *data, size_t len)
{
    if (handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    BaseType_t ret = xRingbufferSend(handle, data, len, 0);
    if (ret != pdTRUE) {
        ESP_LOGE(TAG, "Failed to send data to ring buffer");
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}

/**
 * @brief Receive data from ring buffer
 */
esp_err_t ringbuffer_receive(RingbufHandle_t handle, void *data, size_t *len, uint32_t timeout_ms)
{
    if (handle == NULL || data == NULL || len == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t item_size;
    void *item = xRingbufferReceive(handle, &item_size, pdMS_TO_TICKS(timeout_ms));
    
    if (item == NULL) {
        *len = 0;
        return ESP_ERR_TIMEOUT;
    }

    // Copy data to user buffer
    size_t copy_len = (*len < item_size) ? *len : item_size;
    memcpy(data, item, copy_len);
    *len = copy_len;

    // Return item to ring buffer
    vRingbufferReturnItem(handle, item);

    return ESP_OK;
}

/**
 * @brief Delete ring buffer
 */
esp_err_t ringbuffer_delete(RingbufHandle_t *handle)
{
    if (handle == NULL || *handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    vRingbufferDelete(*handle);
    *handle = NULL;

    ESP_LOGI(TAG, "Ring buffer deleted");
    return ESP_OK;
}

