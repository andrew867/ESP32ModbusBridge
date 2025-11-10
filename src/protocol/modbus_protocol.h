/**
 * @file modbus_protocol.h
 * @brief Modbus protocol frame building
 * 
 * This module provides functions for building Modbus protocol frames.
 * Original: sub_42015834
 */

#ifndef MODBUS_PROTOCOL_H
#define MODBUS_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "function_codes.h"
#include "crc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build a Modbus frame
 * 
 * Original: sub_42015834
 * Builds a complete Modbus frame with address, function code, data, and CRC.
 * 
 * @param buffer Output buffer for the frame
 * @param buffer_size Size of the output buffer
 * @param slave_addr Slave address
 * @param func_code Function code
 * @param data Data to include in frame
 * @param data_len Length of data
 * @param actual_len Pointer to store actual frame length
 * @return 0 on success, -1 on error
 */
int modbus_build_frame(uint8_t *buffer, size_t buffer_size,
                       uint8_t slave_addr, uint8_t func_code,
                       const uint8_t *data, uint16_t data_len,
                       uint16_t *actual_len);

#ifdef __cplusplus
}
#endif

#endif // MODBUS_PROTOCOL_H

