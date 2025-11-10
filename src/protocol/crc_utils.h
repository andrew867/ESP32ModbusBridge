/**
 * @file crc_utils.h
 * @brief CRC calculation utilities
 * 
 * This module provides CRC calculation functions for Modbus protocol.
 * Original: sub_420157EE
 */

#ifndef CRC_UTILS_H
#define CRC_UTILS_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate Modbus CRC-16
 * 
 * Original: sub_420157EE
 * Calculates CRC-16 for Modbus protocol using the standard polynomial.
 * 
 * @param data Data buffer
 * @param length Data length in bytes
 * @return CRC-16 value
 */
uint16_t modbus_crc16(const uint8_t *data, uint16_t length);

/**
 * @brief Verify Modbus CRC
 * 
 * Verifies that the CRC in a Modbus frame is correct.
 * 
 * @param frame Complete Modbus frame including CRC
 * @param length Frame length including CRC (2 bytes)
 * @return 0 if CRC is valid, -1 otherwise
 */
int modbus_verify_crc(const uint8_t *frame, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif // CRC_UTILS_H

