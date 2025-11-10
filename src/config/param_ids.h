/**
 * @file param_ids.h
 * @brief Parameter ID definitions
 * 
 * This file defines all parameter IDs used in the system.
 * Parameters are stored in NVS and can be retrieved/set by ID.
 */

#ifndef PARAM_IDS_H
#define PARAM_IDS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Parameter IDs (0-15)
 * 
 * These correspond to the parameter IDs used in the original code.
 * Each parameter can be either a string or an integer value.
 */
typedef enum {
    PARAM_ID_0 = 0,   // Reserved/Unknown
    PARAM_ID_1 = 1,   // Reserved/Unknown
    PARAM_ID_2 = 2,   // WiFi SSID (string)
    PARAM_ID_3 = 3,   // WiFi Password (string)
    PARAM_ID_4 = 4,   // Reserved/Unknown
    PARAM_ID_5 = 5,   // Server IP/Hostname (string)
    PARAM_ID_6 = 6,   // Server Port (string or int)
    PARAM_ID_7 = 7,   // Device Serial Number (string)
    PARAM_ID_8 = 8,   // Query Period (int, milliseconds)
    PARAM_ID_9 = 9,   // Device ID/Name (string)
    PARAM_ID_10 = 10, // Factory Test Flag (int, 0 or 1)
    PARAM_ID_11 = 11, // Reserved/Unknown
    PARAM_ID_12 = 12, // Reserved/Unknown
    PARAM_ID_13 = 13, // Reserved/Unknown
    PARAM_ID_14 = 14, // IP Configuration (int)
    PARAM_ID_15 = 15, // Reserved/Unknown
    PARAM_ID_MAX = 16
} param_id_t;

#ifdef __cplusplus
}
#endif

#endif // PARAM_IDS_H

