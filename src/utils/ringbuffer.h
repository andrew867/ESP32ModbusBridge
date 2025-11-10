/**
 * @file ringbuffer.h
 * @brief Ring buffer management utilities
 * 
 * This module provides wrapper functions for FreeRTOS ring buffers,
 * commonly used for UART data buffering.
 */

#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include "esp_err.h"
#include "freertos/ringbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a ring buffer for UART data
 * 
 * Creates a ring buffer suitable for UART receive data.
 * 
 * @param size Buffer size in bytes
 * @param handle Pointer to store ring buffer handle
 * @return ESP_OK on success
 */
esp_err_t ringbuffer_create_uart(size_t size, RingbufHandle_t *handle);

/**
 * @brief Send data to ring buffer
 * 
 * @param handle Ring buffer handle
 * @param data Data to send
 * @param len Data length
 * @return ESP_OK on success
 */
esp_err_t ringbuffer_send(RingbufHandle_t handle, const void *data, size_t len);

/**
 * @brief Receive data from ring buffer
 * 
 * @param handle Ring buffer handle
 * @param data Buffer to store received data
 * @param len Pointer to store received length (input: max length, output: actual length)
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK on success, ESP_ERR_TIMEOUT on timeout
 */
esp_err_t ringbuffer_receive(RingbufHandle_t handle, void *data, size_t *len, uint32_t timeout_ms);

/**
 * @brief Delete ring buffer
 * 
 * @param handle Ring buffer handle
 * @return ESP_OK on success
 */
esp_err_t ringbuffer_delete(RingbufHandle_t *handle);

#ifdef __cplusplus
}
#endif

#endif // RINGBUFFER_H

