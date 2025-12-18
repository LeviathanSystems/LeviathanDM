#!/bin/bash
# Test script for the help window
# Note: This requires a running Wayland compositor with layer-shell support

echo "Testing leviathan-help..."
echo ""

# Check if binary exists
if [ ! -f "build/tools/help-window/leviathan-help" ]; then
    echo "ERROR: leviathan-help binary not found!"
    echo "Run ./build.sh first"
    exit 1
fi

echo "✓ Binary found: build/tools/help-window/leviathan-help"

# Check GTK4 dependency
if ! pkg-config --exists gtk4; then
    echo "WARNING: GTK4 not found in pkg-config"
    echo "Install: sudo apt install libgtk-4-dev (or equivalent)"
else
    echo "✓ GTK4 found: $(pkg-config --modversion gtk4)"
fi

# Check gtk-layer-shell dependency
if ! pkg-config --exists gtk-layer-shell-0; then
    echo "WARNING: gtk-layer-shell not found in pkg-config"
    echo "Install: sudo apt install libgtk-layer-shell-dev (or equivalent)"
else
    echo "✓ gtk-layer-shell found: $(pkg-config --modversion gtk-layer-shell-0)"
fi

# Check if running in Wayland
if [ -z "$WAYLAND_DISPLAY" ]; then
    echo ""
    echo "WARNING: WAYLAND_DISPLAY not set"
    echo "This tool requires a Wayland compositor with layer-shell support"
    echo ""
    echo "To test:"
    echo "  1. Start LeviathanDM compositor from a TTY"
    echo "  2. Press Super + F1 to open help window"
    echo ""
    echo "Or run manually in any Wayland compositor with layer-shell:"
    echo "  ./build/tools/help-window/leviathan-help"
else
    echo "✓ Running in Wayland session: $WAYLAND_DISPLAY"
    echo ""
    read -p "Launch help window now? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        ./build/tools/help-window/leviathan-help
    fi
fi
