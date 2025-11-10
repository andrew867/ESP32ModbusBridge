/**
 * @file function_codes.h
 * @brief Protocol function code definitions
 * 
 * This file defines all function codes used in the application protocol.
 * The protocol appears to be Modbus-like with custom function codes.
 */

#ifndef FUNCTION_CODES_H
#define FUNCTION_CODES_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Application protocol function codes
 * 
 * These are the function codes used in the application protocol,
 * which appears to be Modbus-like but with custom codes.
 */
typedef enum {
    // Modbus standard function codes
    MODBUS_FC_READ_HOLDING_REGISTERS = 0x03,
    MODBUS_FC_READ_INPUT_REGISTERS = 0x04,
    
    // Custom function codes (from original code)
    MODBUS_FC_CUSTOM_21 = 0x21,  // Custom function code 21
    MODBUS_FC_CUSTOM_22 = 0x22,  // Custom function code 22
    MODBUS_FC_CUSTOM_88 = 0x88,  // Custom function code 88
    MODBUS_FC_CUSTOM_FE = 0xFE,  // Custom function code FE
    
    // Application protocol function codes
    PROTOCOL_FC_HEARTBEAT = 193,      // Heartbeat message (0xC1)
    PROTOCOL_FC_DATA_TRANSMISSION = 194,  // Data transmission (0xC2)
    PROTOCOL_FC_GET_PARAM = 195,      // Get parameter (0xC3)
    PROTOCOL_FC_SET_PARAM = 196,      // Set parameter (0xC4)
} protocol_function_code_t;

/**
 * @brief Protocol frame structure offsets
 */
#define PROTOCOL_HEADER_SIZE 20
#define PROTOCOL_FUNC_CODE_OFFSET 0
#define PROTOCOL_LENGTH_OFFSET 4
#define PROTOCOL_DATA_LENGTH_OFFSET 18
#define PROTOCOL_DATA_OFFSET 20

#ifdef __cplusplus
}
#endif

#endif // FUNCTION_CODES_H

