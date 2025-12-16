#!/bin/bash
# Build and install libxcb-errors for Debian 12
# This is needed by wlroots but not available in Debian 12 repos

set -e

echo "Building libxcb-errors from source..."

# Install build dependencies
sudo apt install -y \
    git \
    meson \
    ninja-build \
    libxcb1-dev \
    xcb-proto \
    python3-xcbgen

# Create temp directory
TEMP_DIR=$(mktemp -d)
cd "$TEMP_DIR"

# Clone libxcb-errors
echo "Cloning libxcb-errors..."
git clone https://gitlab.freedesktop.org/xorg/lib/libxcb-errors.git
cd libxcb-errors

# Checkout stable version
git checkout xcb-util-errors-1.0.1 || git checkout xcb-errors-1.0 || echo "Using latest version"

# Build and install
echo "Building..."
meson setup build --prefix=/usr
meson compile -C build
sudo meson install -C build

# Clean up
cd /
rm -rf "$TEMP_DIR"

echo "✓ libxcb-errors installed successfully!"
ldconfig -p | grep libxcb-errors
