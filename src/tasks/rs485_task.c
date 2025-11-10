/**
 * @file rs485_task.c
 * @brief RS485/Modbus service task implementation
 * 
 * Original: sub_420136F8 (rs485_service_task, priority 10)
 * 
 * This task handles:
 * - UART initialization for RS485 half-duplex mode
 * - Modbus frame reception and transmission
 * - CRC validation
 * - Function code processing (0x03, 0x04, 0x21, 0x22, 0x88, 0xFE)
 * - Frame timeout handling
 */

#include "rs485_task.h"
#include "../protocol/modbus_protocol.h"
#include "../protocol/crc_utils.h"
#include "../protocol/function_codes.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "rs485_task";

#define RS485_UART_NUM           UART_NUM_2
#define RS485_RX_BUF_SIZE        512
#define RS485_TX_BUF_SIZE        0
#define RS485_BAUD_RATE          9600
#define RS485_DATA_BITS          UART_DATA_8_BITS
#define RS485_PARITY             UART_PARITY_EVEN
#define RS485_STOP_BITS          UART_STOP_BITS_1
#define RS485_RX_TIMEOUT         5  // 5ms timeout
#define RS485_TX_PIN             17
#define RS485_RX_PIN             16
#define RS485_RTS_PIN            4

// RS485 service structure
typedef struct {
    uart_port_t uart_num;
    uint8_t tx_pin;
    uint8_t rx_pin;
    uint8_t rts_pin;
    uint32_t rx_buf_size;
    uint32_t rx_timeout;
    uint8_t *rx_buffer;
    QueueHandle_t frame_queue;
    void (*frame_callback)(uint8_t *frame, size_t len);
} rs485_service_t;

static rs485_service_t s_rs485_service = {0};

/**
 * @brief RS485 service task
 * 
 * Original: sub_420136F8
 * Main loop that:
 * 1. Waits for data from UART
 * 2. Validates CRC
 * 3. Processes function codes
 * 4. Sends responses
 */
static void rs485_service_task(void *pvParameters)
{
    rs485_service_t *service = (rs485_service_t *)pvParameters;
    uint8_t *rx_buffer = service->rx_buffer;
    int len;
    uint16_t crc;
    uint16_t frame_crc;
    uint8_t func_code;
    int retry_count = 0;
    const int max_retries = 50;

    ESP_LOGI(TAG, "RS485 service task started on UART%d", service->uart_num);

    while (1) {
        // Read data from UART with timeout
        len = uart_read_bytes(service->uart_num, rx_buffer, service->rx_buf_size,
                             pdMS_TO_TICKS(service->rx_timeout * 100));

        if (len <= 0) {
            // Timeout - check if we should retry
            if (retry_count < max_retries) {
                retry_count++;
                vTaskDelay(pdMS_TO_TICKS(10));
                continue;
            } else {
                ESP_LOGD(TAG, "RS485 receive timeout");
                retry_count = 0;
                continue;
            }
        }

        retry_count = 0;

        // Minimum frame size is 4 bytes (addr + func + 2 CRC bytes)
        if (len < 4) {
            ESP_LOGW(TAG, "Frame too short: %d bytes", len);
            continue;
        }

        // Extract CRC from frame (last 2 bytes, little-endian)
        frame_crc = rx_buffer[len - 2] | (rx_buffer[len - 1] << 8);

        // Calculate CRC for frame without CRC bytes
        crc = modbus_crc16(rx_buffer, len - 2);

        // Validate CRC
        if (crc != frame_crc) {
            ESP_LOGW(TAG, "CRC mismatch: calculated=0x%04X, received=0x%04X", crc, frame_crc);
            continue;
        }

        // Extract function code
        func_code = rx_buffer[1];

        ESP_LOGD(TAG, "Received valid Modbus frame: addr=0x%02X, func=0x%02X, len=%d",
                 rx_buffer[0], func_code, len);

        // Process function code
        switch (func_code) {
            case MODBUS_FC_READ_HOLDING_REGISTERS:
            case MODBUS_FC_READ_INPUT_REGISTERS:
            case MODBUS_FC_CUSTOM_21:
            case MODBUS_FC_CUSTOM_22:
            case MODBUS_FC_CUSTOM_88:
            case MODBUS_FC_CUSTOM_FE:
                // Call frame callback if registered
                if (service->frame_callback) {
                    service->frame_callback(rx_buffer, len);
                }
                break;

            default:
                ESP_LOGW(TAG, "Unsupported function code: 0x%02X", func_code);
                break;
        }
    }
}

/**
 * @brief Initialize RS485 task
 */
esp_err_t rs485_task_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = RS485_BAUD_RATE,
        .data_bits = RS485_DATA_BITS,
        .parity = RS485_PARITY,
        .stop_bits = RS485_STOP_BITS,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_driver_install(RS485_UART_NUM, RS485_RX_BUF_SIZE * 2,
                                        RS485_TX_BUF_SIZE, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(RS485_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(RS485_UART_NUM, RS485_TX_PIN, RS485_RX_PIN,
                                 RS485_RTS_PIN, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_set_mode(RS485_UART_NUM, UART_MODE_RS485_HALF_DUPLEX));
    ESP_ERROR_CHECK(uart_set_rx_timeout(RS485_UART_NUM, RS485_RX_TIMEOUT));

    // Allocate receive buffer
    s_rs485_service.rx_buffer = malloc(RS485_RX_BUF_SIZE);
    if (s_rs485_service.rx_buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate RX buffer");
        return ESP_ERR_NO_MEM;
    }

    // Initialize service structure
    s_rs485_service.uart_num = RS485_UART_NUM;
    s_rs485_service.tx_pin = RS485_TX_PIN;
    s_rs485_service.rx_pin = RS485_RX_PIN;
    s_rs485_service.rts_pin = RS485_RTS_PIN;
    s_rs485_service.rx_buf_size = RS485_RX_BUF_SIZE;
    s_rs485_service.rx_timeout = RS485_RX_TIMEOUT;
    s_rs485_service.frame_callback = NULL;

    // Create RS485 service task (priority 10)
    BaseType_t ret = xTaskCreate(rs485_service_task, "rs485_service", 4096,
                                  &s_rs485_service, 10, NULL);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create RS485 task");
        free(s_rs485_service.rx_buffer);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "RS485 task initialized");
    return ESP_OK;
}

/**
 * @brief Set frame callback
 */
void rs485_task_set_callback(rs485_frame_callback_t callback)
{
    s_rs485_service.frame_callback = callback;
}

/**
 * @brief Send Modbus frame
 */
esp_err_t rs485_task_send_frame(const uint8_t *frame, size_t len)
{
    if (frame == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    int bytes_written = uart_write_bytes(s_rs485_service.uart_num, frame, len);
    if (bytes_written != len) {
        ESP_LOGE(TAG, "Failed to send frame: wrote %d/%zu bytes", bytes_written, len);
        return ESP_FAIL;
    }

    // Wait for transmission to complete
    esp_err_t ret = uart_wait_tx_done(s_rs485_service.uart_num, pdMS_TO_TICKS(1000));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for TX done: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGD(TAG, "Sent Modbus frame: %zu bytes", len);
    return ESP_OK;
}

