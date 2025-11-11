#!/bin/bash
# Flash script for ESP32ModbusBridge (Linux/macOS)
# Flashes the built firmware to ESP32 over USB

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}ESP32ModbusBridge Flash Script${NC}"
echo "=================================="

# Get port from argument or use default
PORT=${1:-/dev/ttyUSB0}
CHIP=${2:-esp32}

echo "Port: $PORT"
echo "Chip: $CHIP"

# Change to script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Check if build directory exists
if [ ! -d "build" ]; then
    echo -e "${RED}Error: Build directory not found. Please run build.sh first.${NC}"
    exit 1
fi

# Detect ESP-IDF installation
if [ -z "$IDF_PATH" ]; then
    # Try common installation locations
    if [ -d "$HOME/esp/esp-idf" ]; then
        export IDF_PATH="$HOME/esp/esp-idf"
    elif [ -d "/opt/esp/idf" ]; then
        export IDF_PATH="/opt/esp/idf"
    elif [ -d "$HOME/.espressif/esp-idf" ]; then
        export IDF_PATH="$HOME/.espressif/esp-idf"
    else
        echo -e "${RED}Error: ESP-IDF not found.${NC}"
        echo "Please set IDF_PATH environment variable."
        exit 1
    fi
fi

# Source ESP-IDF environment
if [ -f "$IDF_PATH/export.sh" ]; then
    . "$IDF_PATH/export.sh" > /dev/null 2>&1
fi

# Check if port exists
if [ ! -e "$PORT" ]; then
    echo -e "${YELLOW}Warning: Port $PORT not found.${NC}"
    echo "Available ports:"
    ls -1 /dev/ttyUSB* /dev/ttyACM* /dev/cu.* 2>/dev/null | head -5 || echo "  (none found)"
    exit 1
fi

# Find main application binary
MAIN_BIN=$(find build -maxdepth 1 -name "*.bin" -type f ! -name "bootloader.bin" ! -name "partition-table.bin" | head -1)

if [ -z "$MAIN_BIN" ]; then
    echo -e "${RED}Error: Main application binary not found in build directory.${NC}"
    exit 1
fi

echo -e "${GREEN}Flashing firmware to $PORT...${NC}"
echo ""

# Flash using esptool.py
python "$IDF_PATH/components/esptool_py/esptool/esptool.py" \
  --chip "$CHIP" \
  --port "$PORT" \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  -z --flash_mode dio --flash_freq 40m --flash_size 4MB \
  0x1000 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 "$MAIN_BIN"

if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}Flash complete!${NC}"
    echo ""
    echo "To monitor serial output:"
    echo "  idf.py -p $PORT monitor"
    echo ""
    echo "Or press Ctrl+] to exit monitor"
else
    echo -e "${RED}Flash failed!${NC}"
    exit 1
fi

