/**
 * @file uart_rx_task.c
 * @brief UART RX task implementation
 * 
 * Original: sub_42013DE6 (uart_rx_task, priority 5)
 * 
 * This task handles:
 * - UART initialization for terminal service
 * - Continuous data reception
 * - Data forwarding to callback
 */

#include "uart_rx_task.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

static const char *TAG = "uart_rx_task";

#define UART_RX_UART_NUM          UART_NUM_1
#define UART_RX_RX_BUF_SIZE       512
#define UART_RX_TX_BUF_SIZE       0
#define UART_RX_BAUD_RATE         115200
#define UART_RX_DATA_BITS         UART_DATA_8_BITS
#define UART_RX_PARITY            UART_PARITY_DISABLE
#define UART_RX_STOP_BITS         UART_STOP_BITS_1
#define UART_RX_RX_TIMEOUT        5  // 5ms timeout
#define UART_RX_TX_PIN            1
#define UART_RX_RX_PIN            3

// UART RX callback type
typedef void (*uart_rx_callback_t)(uint8_t *data, size_t len);

static uart_rx_callback_t s_rx_callback = NULL;

/**
 * @brief UART RX task
 * 
 * Original: sub_42013DE6
 * Continuously reads from UART and forwards data to callback
 */
static void uart_rx_task(void *pvParameters)
{
    uint8_t *rx_buffer = malloc(UART_RX_RX_BUF_SIZE);
    if (rx_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RX buffer");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "UART RX task started on UART%d", UART_RX_UART_NUM);

    while (1) {
        int len = uart_read_bytes(UART_RX_UART_NUM, rx_buffer, UART_RX_RX_BUF_SIZE,
                                 pdMS_TO_TICKS(UART_RX_RX_TIMEOUT * 100));

        if (len > 0) {
            // Forward data to callback if registered
            if (s_rx_callback) {
                s_rx_callback(rx_buffer, len);
            }
        } else if (len < 0 && errno != EAGAIN) {
            ESP_LOGE(TAG, "UART read error: %d", errno);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    free(rx_buffer);
    vTaskDelete(NULL);
}

/**
 * @brief Initialize UART RX task
 * 
 * Original: sub_42013DE6 initialization
 */
esp_err_t uart_rx_task_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = UART_RX_BAUD_RATE,
        .data_bits = UART_RX_DATA_BITS,
        .parity = UART_RX_PARITY,
        .stop_bits = UART_RX_STOP_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_driver_install(UART_RX_UART_NUM, UART_RX_RX_BUF_SIZE * 2,
                                        UART_RX_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART_RX_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_RX_UART_NUM, UART_RX_TX_PIN, UART_RX_RX_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(UART_RX_UART_NUM, UART_MODE_UART));
    ESP_ERROR_CHECK(uart_set_rx_timeout(UART_RX_UART_NUM, UART_RX_RX_TIMEOUT));

    // Create UART RX task (priority 5)
    BaseType_t ret = xTaskCreate(uart_rx_task, "uart_rx", 2048, NULL, 5, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create UART RX task");
        uart_driver_delete(UART_RX_UART_NUM);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "UART RX task initialized on UART%d", UART_RX_UART_NUM);
    return ESP_OK;
}

/**
 * @brief Set RX callback
 */
void uart_rx_task_set_callback(void (*callback)(uint8_t *data, size_t len))
{
    s_rx_callback = callback;
}

