/**
 * @file modbus_protocol.c
 * @brief Modbus protocol implementation
 */

#include "modbus_protocol.h"
#include <string.h>

/**
 * @brief Build a Modbus frame
 * 
 * Original: sub_42015834
 * Standard Modbus RTU frame format:
 * [Slave Address] [Function Code] [Data] [CRC Low] [CRC High]
 */
int modbus_build_frame(uint8_t *buffer, size_t buffer_size,
                       uint8_t slave_addr, uint8_t func_code,
                       const uint8_t *data, uint16_t data_len,
                       uint16_t *actual_len)
{
    if (buffer == NULL || buffer_size < 4 + data_len) {
        return -1;
    }

    uint16_t frame_len = 0;

    // Slave address
    buffer[frame_len++] = slave_addr;

    // Function code
    buffer[frame_len++] = func_code;

    // Data
    if (data != NULL && data_len > 0) {
        memcpy(&buffer[frame_len], data, data_len);
        frame_len += data_len;
    }

    // Calculate CRC
    uint16_t crc = modbus_crc16(buffer, frame_len);

    // Append CRC (little-endian)
    buffer[frame_len++] = crc & 0xFF;
    buffer[frame_len++] = (crc >> 8) & 0xFF;

    if (actual_len != NULL) {
        *actual_len = frame_len;
    }

    return 0;
}

