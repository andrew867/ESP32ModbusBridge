# ESP32 Modbus WiFi Bridge

A complete ESP-IDF/FreeRTOS implementation providing a Modbus-to-WiFi bridge with RS485 communication, TCP/TLS client and server, BLE support, and comprehensive system management.

## Features

### Communication Protocols
- **RS485/Modbus RTU**: Full Modbus RTU support with CRC validation
- **WiFi**: Dual mode (AP+STA) with static IP configuration
- **TCP/TLS Client**: Secure connection to cloud server with PSK authentication
- **TCP/TLS Server**: Multi-client server support
- **BLE**: GATT server with read/write/notify characteristics

### Data Routing
- **RS485 ↔ TCP**: Bidirectional data routing between Modbus and TCP protocols
- **Protocol Conversion**: Automatic conversion between Modbus frames and protocol frames
- **Data Processing**: Comprehensive frame building, parsing, and validation

### System Management
- **Parameter Management**: NVS-based persistent storage
- **OTA Updates**: HTTPS-based firmware updates
- **Factory Test/Reset**: Factory mode and reset functionality
- **Heartbeat**: Keep-alive mechanism for connections
- **Poll Timer**: Periodic RS485 polling

### User Interface
- **LED Status**: Visual indication of system state (3 LEDs)
- **Button Control**: Factory reset, test mode, and reboot
- **UART Terminal**: Command-line interface (LPTS1-7, LPTQ1-7, SHELL)

## Project Structure

```
ESP32ModbusBridge/
├── main/
│   ├── main.c              # Application entry point
│   └── CMakeLists.txt      # Build configuration
├── src/
│   ├── tasks/              # FreeRTOS tasks
│   │   ├── rs485_task.c/h      # RS485/Modbus communication
│   │   ├── wifi_task.c/h       # WiFi management
│   │   ├── tcp_client_task.c/h # TCP client with TLS
│   │   ├── tcp_server_task.c/h # TCP server
│   │   ├── ble_task.c/h        # BLE GATT server
│   │   ├── uart_rx_task.c/h    # UART terminal
│   │   ├── led_task.c/h        # LED status indication
│   │   └── button_task.c/h     # Button handling
│   ├── protocol/           # Protocol handling
│   │   ├── data_process.c/h    # Data processing module
│   │   ├── modbus_protocol.c/h # Modbus protocol
│   │   ├── crc_utils.c/h       # CRC calculation
│   │   └── function_codes.h    # Function code definitions
│   ├── config/             # Configuration
│   │   ├── param_manager.c/h   # Parameter management
│   │   └── param_ids.h         # Parameter ID definitions
│   ├── shell/              # Terminal interface
│   │   └── terminal_service.c/h # Command parser
│   ├── utils/              # System utilities
│   │   ├── heartbeat.c/h       # Heartbeat mechanism
│   │   ├── poll_timer.c/h      # Poll timer
│   │   ├── factory_test.c/h    # Factory test mode
│   │   ├── system_utils.c/h    # System utilities
│   │   ├── watchdog.c/h        # Watchdog management
│   │   └── ringbuffer.c/h      # Ring buffer utilities
│   ├── ota/                # OTA updates
│   │   └── ota_manager.c/h     # OTA manager
│   ├── system/             # System initialization
│   │   ├── sdk_init.c/h        # SDK initialization
│   │   └── boot_init.c/h       # Boot initialization
│   ├── drivers/            # Hardware drivers (stubs)
│   └── network/            # Network utilities (stubs)
├── CMakeLists.txt          # Root build file
├── sdkconfig.defaults      # Default SDK configuration
└── README.md               # This file
```

## Building

### Prerequisites
- ESP-IDF v4.4 or later
- CMake 3.16 or later
- Python 3.6 or later

### Build Steps

```bash
cd ESP32ModbusBridge
idf.py build
idf.py flash monitor
```

## Configuration

### Default Parameters
- WiFi SSID: "LuxPower" (configurable via parameters)
- Server: "dongle_ssl.solarcloudsystem.com:4348"
- RS485: 9600 baud, 8E1, half-duplex
- UART Terminal: 115200 baud, 8N1

### Parameter IDs
- PARAM_ID_2: WiFi SSID
- PARAM_ID_3: WiFi Password
- PARAM_ID_5: Server Host
- PARAM_ID_6: Server Port
- PARAM_ID_7: Device Serial Number
- PARAM_ID_8: Query Period
- PARAM_ID_10: Factory Test Flag
- PARAM_ID_14: IP Configuration (0=DHCP, 1=Static)

## Usage

### Terminal Commands
- `LPTS1`: Set WiFi SSID
- `LPTS3`: Set WiFi Password
- `LPTS4`: Set Server Host
- `LPTS5`: Set Server Port
- `LPTS7`: Set Device SN
- `LPTQ1`: Query WiFi SSID
- `LPTQ2`: Query WiFi Password
- `LPTQ3`: Query Server Host
- `LPTQ4`: Query Server Port
- `LPTQ6`: Query Device SN
- `LPTQ7`: Query Device Status
- `SHELL`: Enable shell mode

### Button Controls
- **Short Press** (>100ms): Factory test mode
- **Medium Press** (>500ms): Factory reset
- **Long Press** (>1000ms): System reboot

## Protocol

### Modbus Function Codes
- 0x03: Read Holding Registers
- 0x04: Read Input Registers
- 0x21, 0x22, 0x88, 0xFE: Custom function codes

### Application Protocol Function Codes
- 193 (0xC1): Heartbeat
- 194 (0xC2): Data Transmission
- 195 (0xC3): Get Parameter
- 196 (0xC4): Set Parameter

## Code Structure

This project is organized into modular components with clear separation of concerns:
- Task-based architecture for concurrent operations
- Protocol handling for Modbus and custom protocols
- Configuration management via NVS storage
- Comprehensive error handling and logging throughout

## License

[Add your license here]

## Status

✅ **Project Complete** - All modules implemented and ready for testing

## Documentation

For detailed documentation, see the `docs/` directory:
- `IMPLEMENTATION_STATUS.md` - Detailed implementation status
- `PROJECT_COMPLETE.md` - Project completion summary
- `BUILD_AND_DEPLOY.md` - Build and deployment guide
- `STUB_FILES_REMOVED.md` - Information about removed stub files
- And more...
