#!/bin/bash

# Setup script for vendored dependencies
# Downloads specific versions of wlroots and libdisplay-info

set -e

echo "Setting up vendored dependencies..."

# Create subprojects directory
mkdir -p subprojects

# Clone wlroots 0.19.2
if [ ! -d "subprojects/wlroots" ]; then
    echo "Cloning wlroots 0.19.2..."
    git clone https://gitlab.freedesktop.org/wlroots/wlroots.git subprojects/wlroots
    cd subprojects/wlroots
    git checkout 0.19.2
    cd ../..
else
    echo "wlroots already exists, checking out 0.19.2..."
    cd subprojects/wlroots
    git fetch
    git checkout 0.19.2
    cd ../..
fi

# Clone libdisplay-info (latest stable: 0.2.0)
if [ ! -d "subprojects/libdisplay-info" ]; then
    echo "Cloning libdisplay-info 0.2.0..."
    git clone https://gitlab.freedesktop.org/emersion/libdisplay-info.git subprojects/libdisplay-info
    cd subprojects/libdisplay-info
    git checkout 0.2.0
    cd ../..
else
    echo "libdisplay-info already exists, checking out 0.2.0..."
    cd subprojects/libdisplay-info
    git fetch
    git checkout 0.2.0
    cd ../..
fi

# Clone wayland-protocols for wlroots
if [ ! -d "subprojects/wlroots/subprojects/wayland-protocols" ]; then
    echo "Cloning wayland-protocols for wlroots..."
    git clone https://gitlab.freedesktop.org/wayland/wayland-protocols.git subprojects/wlroots/subprojects/wayland-protocols
else
    echo "wayland-protocols already exists"
fi

echo ""
echo "✓ Dependencies downloaded successfully!"
echo "  - wlroots: 0.19.2"
echo "  - libdisplay-info: 0.2.0"
echo "  - wayland-protocols: (wlroots subproject)"
echo ""
echo "Run ./build.sh to build with vendored dependencies"
