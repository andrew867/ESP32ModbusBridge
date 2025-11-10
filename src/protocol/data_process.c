/**
 * @file data_process.c
 * @brief Data processing implementation
 */

#include "data_process.h"
#include "crc_utils.h"
#include "function_codes.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

static const char *TAG = "data_process";

struct data_process_handle {
    void (*send_callback)(const uint8_t *data, size_t len);
    void (*receive_callback)(const uint8_t *data, size_t len);
};

data_process_handle_t data_process_create(void (*send_callback)(const uint8_t *data, size_t len),
                                          void (*receive_callback)(const uint8_t *data, size_t len))
{
    struct data_process_handle *handle = malloc(sizeof(struct data_process_handle));
    if (handle == NULL) {
        return NULL;
    }

    handle->send_callback = send_callback;
    handle->receive_callback = receive_callback;

    return handle;
}

esp_err_t data_process_receive(data_process_handle_t handle, const uint8_t *data, size_t len)
{
    if (handle == NULL || data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Process received data and call callback
    if (handle->receive_callback != NULL) {
        handle->receive_callback(data, len);
    }

    return ESP_OK;
}

// Protocol frame sequence number (incremented for each frame)
static uint16_t s_frame_sequence = 0;

/**
 * @brief Build protocol frame header
 * 
 * Original: sub_42013470
 * Builds the base protocol frame header
 * Header format: [0xA1][0x1A][seq(2)][0][0][1][func_code][data(10)]
 */
static int build_protocol_header(uint8_t *buffer, uint8_t func_code, const uint8_t *header_data)
{
    if (buffer == NULL) {
        return -1;
    }

    // Protocol header
    buffer[0] = 0xA1;  // Protocol identifier
    buffer[1] = 0x1A;  // Protocol version/type
    buffer[2] = s_frame_sequence & 0xFF;
    buffer[3] = (s_frame_sequence >> 8) & 0xFF;
    buffer[4] = 0;
    buffer[5] = 0;
    buffer[6] = 1;
    buffer[7] = func_code;
    
    // Copy header data (10 bytes)
    if (header_data != NULL) {
        memcpy(&buffer[8], header_data, 10);
    } else {
        memset(&buffer[8], 0, 10);
    }
    
    s_frame_sequence++;
    if (s_frame_sequence == 0) {
        s_frame_sequence = 1;  // Skip 0
    }
    
    return 0;
}

/**
 * @brief Build data transmission frame (function code 194)
 * 
 * Original: sub_4201357E
 * Frame format: [header(18)][data_len(2)][data][crc(2)]
 */
static int build_data_transmission_frame(uint8_t *buffer, size_t buffer_size,
                                         const uint8_t *data, uint16_t data_len,
                                         uint16_t *actual_len)
{
    if (buffer == NULL || buffer_size < 20 + data_len) {
        return -1;
    }

    // Build header
    build_protocol_header(buffer, PROTOCOL_FC_DATA_TRANSMISSION, NULL);
    
    // Data length (bytes 18-19, little-endian)
    buffer[18] = data_len & 0xFF;
    buffer[19] = (data_len >> 8) & 0xFF;
    
    // Copy data
    if (data != NULL && data_len > 0) {
        memcpy(&buffer[20], data, data_len);
    }
    
    // Calculate CRC for frame without CRC bytes
    uint16_t frame_len = 20 + data_len;
    uint16_t crc = modbus_crc16(buffer, frame_len);
    buffer[frame_len] = crc & 0xFF;
    buffer[frame_len + 1] = (crc >> 8) & 0xFF;
    
    if (actual_len != NULL) {
        *actual_len = frame_len + 2;
    }
    
    return 0;
}

/**
 * @brief Build get parameter frame (function code 195)
 * 
 * Original: sub_420134AE
 * Frame format: [header(18)][param_id(2)][end_param(2)][data][crc(2)]
 */
static int build_get_param_frame(uint8_t *buffer, size_t buffer_size,
                                 uint16_t param_id, uint16_t end_param,
                                 const uint8_t *data, uint16_t data_len,
                                 uint16_t *actual_len)
{
    if (buffer == NULL || buffer_size < 22 + data_len) {
        return -1;
    }

    // Build header
    build_protocol_header(buffer, PROTOCOL_FC_GET_PARAM, NULL);
    
    // Parameter ID (bytes 18-19, little-endian)
    buffer[18] = param_id & 0xFF;
    buffer[19] = (param_id >> 8) & 0xFF;
    
    // End parameter ID (bytes 20-21, little-endian)
    buffer[20] = end_param & 0xFF;
    buffer[21] = (end_param >> 8) & 0xFF;
    
    // Copy data if present
    if (data != NULL && data_len > 0) {
        memcpy(&buffer[22], data, data_len);
    }
    
    // Calculate CRC
    uint16_t frame_len = 22 + data_len;
    uint16_t crc = modbus_crc16(buffer, frame_len);
    buffer[frame_len] = crc & 0xFF;
    buffer[frame_len + 1] = (crc >> 8) & 0xFF;
    
    if (actual_len != NULL) {
        *actual_len = frame_len + 2;
    }
    
    return 0;
}

/**
 * @brief Build set parameter frame (function code 196)
 * 
 * Original: sub_4201352C
 * Frame format: [header(18)][param_id(2)][data_len(1)][data][crc(2)]
 */
static int build_set_param_frame(uint8_t *buffer, size_t buffer_size,
                                 uint16_t param_id, uint8_t data_len,
                                 const uint8_t *data,
                                 uint16_t *actual_len)
{
    if (buffer == NULL || buffer_size < 21 + data_len) {
        return -1;
    }

    // Build header
    build_protocol_header(buffer, PROTOCOL_FC_SET_PARAM, NULL);
    
    // Parameter ID (bytes 18-19, little-endian)
    buffer[18] = param_id & 0xFF;
    buffer[19] = (param_id >> 8) & 0xFF;
    
    // Data length (byte 20)
    buffer[20] = data_len;
    
    // Copy data
    if (data != NULL && data_len > 0) {
        memcpy(&buffer[21], data, data_len);
    }
    
    // Calculate CRC
    uint16_t frame_len = 21 + data_len;
    uint16_t crc = modbus_crc16(buffer, frame_len);
    buffer[frame_len] = crc & 0xFF;
    buffer[frame_len + 1] = (crc >> 8) & 0xFF;
    
    if (actual_len != NULL) {
        *actual_len = frame_len + 2;
    }
    
    return 0;
}

/**
 * @brief Build heartbeat frame (function code 193)
 * 
 * Original: sub_420135EA
 * Frame format: [header(18)][data_len(1)][crc(2)]
 */
static int build_heartbeat_frame(uint8_t *buffer, size_t buffer_size,
                                 uint16_t *actual_len)
{
    if (buffer == NULL || buffer_size < 20) {
        return -1;
    }

    // Build header
    build_protocol_header(buffer, PROTOCOL_FC_HEARTBEAT, NULL);
    
    // Data length (byte 18)
    buffer[18] = 6;
    
    // Calculate CRC
    uint16_t frame_len = 19;
    uint16_t crc = modbus_crc16(buffer, frame_len);
    buffer[frame_len] = crc & 0xFF;
    buffer[frame_len + 1] = (crc >> 8) & 0xFF;
    
    if (actual_len != NULL) {
        *actual_len = frame_len + 2;
    }
    
    return 0;
}

esp_err_t data_process_send(data_process_handle_t handle, uint8_t func_code, const uint8_t *data, size_t len)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->send_callback == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Build protocol frame based on function code
    uint8_t frame_buffer[512];
    uint16_t frame_len = 0;
    int ret = -1;
    uint16_t param_id = 0;
    uint16_t end_param = 0;
    
    switch (func_code) {
        case PROTOCOL_FC_HEARTBEAT:
            ret = build_heartbeat_frame(frame_buffer, sizeof(frame_buffer), &frame_len);
            break;
            
        case PROTOCOL_FC_DATA_TRANSMISSION:
            ret = build_data_transmission_frame(frame_buffer, sizeof(frame_buffer),
                                                data, (uint16_t)len, &frame_len);
            break;
            
        case PROTOCOL_FC_GET_PARAM: {
            // Extract param_id and end_param from data
            if (len >= 2) {
                param_id = data[0] | (data[1] << 8);
            }
            if (len >= 4) {
                end_param = data[2] | (data[3] << 8);
            }
            ret = build_get_param_frame(frame_buffer, sizeof(frame_buffer),
                                       param_id, end_param,
                                       (len > 4) ? &data[4] : NULL,
                                       (len > 4) ? (uint16_t)(len - 4) : 0,
                                       &frame_len);
            break;
        }
            
        case PROTOCOL_FC_SET_PARAM: {
            // Extract param_id and data from input
            if (len >= 2) {
                param_id = data[0] | (data[1] << 8);
            }
            ret = build_set_param_frame(frame_buffer, sizeof(frame_buffer),
                                        param_id,
                                        (len > 2) ? (uint8_t)(len - 2) : 0,
                                        (len > 2) ? &data[2] : NULL,
                                        &frame_len);
            break;
        }
            
        default:
            ESP_LOGE(TAG, "Unsupported function code: 0x%02X", func_code);
            return ESP_ERR_NOT_SUPPORTED;
    }
    
    if (ret != 0) {
        ESP_LOGE(TAG, "Failed to build protocol frame for function code 0x%02X", func_code);
        return ESP_FAIL;
    }
    
    // Send frame through callback
    handle->send_callback(frame_buffer, frame_len);
    
    ESP_LOGD(TAG, "Sent frame: func_code=0x%02X, len=%u", func_code, frame_len);
    return ESP_OK;
}

int parse_data_transmission_frame(const uint8_t *frame, size_t frame_len, 
                                  uint8_t **data_out, uint16_t *data_len)
{
    if (frame == NULL || frame_len <= 20) {
        return -1;
    }

    // Verify protocol header
    if (frame[0] != 0xA1 || frame[1] != 0x1A || frame[7] != PROTOCOL_FC_DATA_TRANSMISSION) {
        return -1;
    }

    // Extract data length from frame (bytes 18-19, little-endian)
    uint16_t len = frame[18] | (frame[19] << 8);
    
    // Frame should be: 20 bytes (header + data_len) + data + 2 bytes CRC
    if (len + 22 != frame_len) {
        return -1;
    }

    // Verify CRC
    uint16_t calculated_crc = modbus_crc16(frame, frame_len - 2);
    uint16_t frame_crc = frame[frame_len - 2] | (frame[frame_len - 1] << 8);
    if (calculated_crc != frame_crc) {
        return -1;
    }

    *data_len = len;
    *data_out = (uint8_t *)(frame + 20);
    return 0;
}

int parse_set_param_frame(const uint8_t *frame, size_t frame_len,
                          uint16_t *param_id, uint16_t *data_len, uint8_t **data_out)
{
    if (frame == NULL || frame_len <= 21) {
        return -1;
    }

    // Verify protocol header
    if (frame[0] != 0xA1 || frame[1] != 0x1A || frame[7] != PROTOCOL_FC_SET_PARAM) {
        return -1;
    }

    // Extract parameter ID (bytes 18-19, little-endian)
    *param_id = frame[18] | (frame[19] << 8);
    
    // Extract data length (byte 20)
    uint8_t len = frame[20];
    
    // Frame should be: 21 bytes (header + param_id + data_len) + data + 2 bytes CRC
    if (len + 23 != frame_len) {
        return -1;
    }

    // Verify CRC
    uint16_t calculated_crc = modbus_crc16(frame, frame_len - 2);
    uint16_t frame_crc = frame[frame_len - 2] | (frame[frame_len - 1] << 8);
    if (calculated_crc != frame_crc) {
        return -1;
    }

    *data_len = len;
    *data_out = (uint8_t *)(frame + 21);
    return 0;
}

int parse_get_param_frame(const uint8_t *frame, size_t frame_len,
                          uint16_t *param_id, uint16_t *expected_len)
{
    if (frame == NULL || frame_len <= 20) {
        return -1;
    }

    // Verify protocol header
    if (frame[0] != 0xA1 || frame[1] != 0x1A || frame[7] != PROTOCOL_FC_GET_PARAM) {
        return -1;
    }

    // Extract parameter ID (bytes 18-19, little-endian)
    *param_id = frame[18] | (frame[19] << 8);
    
    // Extract end parameter ID (bytes 20-21, little-endian)
    if (frame_len > 22) {
        *expected_len = frame[20] | (frame[21] << 8);
    } else {
        *expected_len = *param_id; // Use param_id as length if not present
    }

    // Verify CRC if frame is complete
    if (frame_len >= 22) {
        uint16_t calculated_crc = modbus_crc16(frame, frame_len - 2);
        uint16_t frame_crc = frame[frame_len - 2] | (frame[frame_len - 1] << 8);
        if (calculated_crc != frame_crc) {
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Destroy data processing module
 */
void data_process_destroy(data_process_handle_t handle)
{
    if (handle != NULL) {
        free(handle);
    }
}

