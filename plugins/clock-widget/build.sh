#!/bin/bash
# Build script for clock-widget plugin

set -e  # Exit on error

echo "Building ClockWidget plugin..."

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo "Configuring..."
cmake ..

# Build
echo "Building..."
make

# Check if build succeeded
if [ -f "clock-widget.so" ]; then
    echo ""
    echo "✓ Build successful!"
    echo "  Output: $BUILD_DIR/clock-widget.so"
    echo ""
    echo "To install:"
    echo "  User:   mkdir -p ~/.config/leviathan/plugins && cp clock-widget.so ~/.config/leviathan/plugins/"
    echo "  System: sudo make install"
else
    echo "✗ Build failed!"
    exit 1
fi
