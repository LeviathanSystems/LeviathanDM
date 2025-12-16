#!/bin/bash

# Build script for LeviathanDM (Wayland Compositor)

set -e

echo "Building LeviathanDM Wayland Compositor..."

# Check for meson (needed for libdisplay-info)
if ! command -v meson &> /dev/null; then
    echo "Error: meson not found!"
    echo "Please install meson:"
    echo "  Ubuntu/Debian: sudo apt install meson"
    echo "  Arch Linux: sudo pacman -S meson"
    echo "  Fedora: sudo dnf install meson"
    exit 1
fi

# Setup vendored dependencies if not already done
if [ ! -d "subprojects/wlroots" ] || [ ! -d "subprojects/libdisplay-info" ]; then
    echo "Setting up vendored dependencies..."
    ./setup-deps.sh
fi

# Create build directory
mkdir -p build
cd build

# Run CMake with vendored dependencies
cmake -DCMAKE_BUILD_TYPE=Release -DUSE_VENDORED_DEPS=ON ..

# Build
make -j$(nproc)

echo ""
echo "Build complete! Binary is at: build/leviathan"
echo ""
echo "Using vendored dependencies:"
echo "  - wlroots 0.19.2"
echo "  - libdisplay-info 0.2.0"
echo ""
echo "To test from a TTY:"
echo "  # Switch to TTY (Ctrl+Alt+F2), login, then:"
echo "  ./build/leviathan"
echo ""
echo "To test in a nested session:"
echo "  cage ./build/leviathan"
echo ""
echo "To install system-wide:"
echo "  sudo make install"
