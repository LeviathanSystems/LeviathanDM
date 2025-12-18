#!/bin/bash
set -e

echo "Building TagListWidget plugin..."

# Create build directory
mkdir -p build
cd build

# Run CMake
cmake ..

# Build
make

echo ""
echo "✓ Plugin built successfully!"
echo ""
echo "To install to user plugins directory:"
echo "  mkdir -p ~/.config/leviathan/plugins"
echo "  cp taglist-widget.so ~/.config/leviathan/plugins/"
echo ""
echo "To install system-wide:"
echo "  sudo make install"
