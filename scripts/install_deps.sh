#!/bin/bash

# ArchMaster Dependency Installer
# This script installs all necessary prerequisites for building ArchMaster on Arch Linux.

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

echo -e "${GREEN}==>${NC} ArchMaster Dependency Installer"

# 1. Check if OS is Arch Linux
if [ ! -f /etc/arch-release ]; then
    echo -e "${RED}Error:${NC} This script is designed for Arch Linux only."
    exit 1
fi

# 2. Check for root privileges
if [ "$EUID" -ne 0 ]; then
    echo -e "${RED}Error:${NC} Please run this script with sudo."
    exit 1
fi

# 3. Update package database
echo -e "${GREEN}==>${NC} Updating package database..."
pacman -Sy

# 4. Install dependencies
# List of dependencies:
# - base-devel: Standard build tools (gcc, make, etc.)
# - cmake: Build system
# - qt6-base, qt6-svg, qt6-charts, qt6-5compat: Qt6 libraries
# - curl: Network operations
# - pkgconf: Package configuration
# - alpm: Arch Linux Package Manager library (usually part of pacman, but good to ensure header availability)

echo -e "${GREEN}==>${NC} Installing dependencies..."
pacman -S --needed --noconfirm base-devel cmake qt6-base qt6-svg qt6-charts qt6-5compat curl pkgconf vulkan-headers

echo -e "${GREEN}==>${NC} Dependencies installed successfully!"
