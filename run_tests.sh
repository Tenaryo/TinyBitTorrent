#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}   Running Test Suite${NC}"
echo -e "${BLUE}========================================${NC}"
echo

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

if [ ! -f "CMakeCache.txt" ]; then
    echo -e "${YELLOW}Configuring CMake...${NC}"
    cmake .. -DCMAKE_BUILD_TYPE=Debug
fi

echo -e "${YELLOW}Building tests...${NC}"
cmake --build . -j$(nproc) 2>&1 | grep -E "(Building|Linking|error|warning)" || true
echo

echo -e "${YELLOW}Running tests via ctest...${NC}"
echo

if ctest --output-on-failure -j$(nproc); then
    echo
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
