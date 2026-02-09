#!/bin/bash

# ArchMaster Setup and Build Script
# This script automates the installation of dependencies and the building of ArchMaster.

set -e

# Colors for output
GREEN='\033[0;32m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo -e "${GREEN}==>${NC} Starting ArchMaster Setup"

# 1. Install dependencies
echo -e "${GREEN}==>${NC} Installing dependencies..."
sudo bash "$SCRIPT_DIR/install_deps.sh"

# 2. Build the project
echo -e "${GREEN}==>${NC} Configuring and building the project..."
cd "$PROJECT_ROOT"

# Clean build directory if it exists (optional, but often good for fresh start)
# rm -rf build

cmake -B build -S .
cmake --build build -j$(nproc)

echo -e "\n${GREEN}==>${NC} Build complete! You can run the application with:"
echo -e "./build/bin/archmaster"
