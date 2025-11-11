#!/bin/bash
# Build script for ESP32ModbusBridge (Linux/macOS)
# Detects ESP-IDF installation and builds the firmware

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}ESP32ModbusBridge Build Script${NC}"
echo "=================================="

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check for Python
if ! command_exists python3 && ! command_exists python; then
    echo -e "${RED}Error: Python not found. Please install Python 3.6 or later.${NC}"
    exit 1
fi

PYTHON_CMD=""
if command_exists python3; then
    PYTHON_CMD="python3"
elif command_exists python; then
    PYTHON_CMD="python"
fi

# Detect ESP-IDF installation
if [ -z "$IDF_PATH" ]; then
    # Try common installation locations
    if [ -d "$HOME/esp/esp-idf" ]; then
        export IDF_PATH="$HOME/esp/esp-idf"
        echo -e "${YELLOW}Found ESP-IDF at: $IDF_PATH${NC}"
    elif [ -d "/opt/esp/idf" ]; then
        export IDF_PATH="/opt/esp/idf"
        echo -e "${YELLOW}Found ESP-IDF at: $IDF_PATH${NC}"
    elif [ -d "$HOME/.espressif/esp-idf" ]; then
        export IDF_PATH="$HOME/.espressif/esp-idf"
        echo -e "${YELLOW}Found ESP-IDF at: $IDF_PATH${NC}"
    else
        echo -e "${RED}Error: ESP-IDF not found.${NC}"
        echo "Please set IDF_PATH environment variable or install ESP-IDF."
        echo "Installation guide: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/"
        exit 1
    fi
else
    echo -e "${GREEN}Using ESP-IDF from IDF_PATH: $IDF_PATH${NC}"
fi

# Verify IDF_PATH exists
if [ ! -d "$IDF_PATH" ]; then
    echo -e "${RED}Error: IDF_PATH directory does not exist: $IDF_PATH${NC}"
    exit 1
fi

# Check if ESP-IDF is properly set up
if [ ! -f "$IDF_PATH/export.sh" ] && [ ! -f "$IDF_PATH/export.bat" ]; then
    echo -e "${RED}Error: ESP-IDF installation appears incomplete at: $IDF_PATH${NC}"
    exit 1
fi

# Source ESP-IDF environment
echo -e "${GREEN}Setting up ESP-IDF environment...${NC}"
if [ -f "$IDF_PATH/export.sh" ]; then
    . "$IDF_PATH/export.sh" > /dev/null 2>&1
else
    echo -e "${YELLOW}Warning: export.sh not found, trying to use idf.py directly${NC}"
fi

# Verify idf.py is available
if ! command_exists idf.py; then
    echo -e "${RED}Error: idf.py not found. Please ensure ESP-IDF is properly installed.${NC}"
    echo "Try running: . \$IDF_PATH/export.sh"
    exit 1
fi

# Get target chip from argument or use default
TARGET=${1:-esp32}
echo -e "${GREEN}Target chip: $TARGET${NC}"

# Change to script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

# Set target
echo -e "${GREEN}Setting target to $TARGET...${NC}"

# Handle build directory
# set-target runs fullclean which requires a valid CMake build directory
# If build dir exists but is invalid (no CMakeCache.txt), delete it first
if [ -d "build" ] && [ ! -f "build/CMakeCache.txt" ]; then
    echo -e "${YELLOW}Removing invalid build directory...${NC}"
    rm -rf build
fi
mkdir -p build

# Set target (this may run fullclean)
idf.py set-target "$TARGET"

# Build the project
# Note: If you change partition table settings, you may need to run:
#   idf.py fullclean
# before building to ensure changes are picked up.
echo -e "${GREEN}Building project...${NC}"
idf.py build

# Check if build was successful
if [ $? -eq 0 ]; then
    echo -e "${GREEN}Build successful!${NC}"
    echo ""
    echo "Build artifacts:"
    echo "  - Bootloader: build/bootloader/bootloader.bin"
    echo "  - Partition table: build/partition_table/partition-table.bin"
    
    # Find main application binary
    MAIN_BIN=$(find build -maxdepth 1 -name "*.bin" -type f ! -name "bootloader.bin" ! -name "partition-table.bin" | head -1)
    if [ -n "$MAIN_BIN" ]; then
        echo "  - Application: $MAIN_BIN"
    fi
    
    echo ""
    echo "To flash the firmware:"
    echo "  idf.py -p /dev/ttyUSB0 flash monitor"
    echo ""
    echo "Or use the flash script:"
    echo "  ./flash.sh /dev/ttyUSB0"
else
    echo -e "${RED}Build failed!${NC}"
    exit 1
fi

