#!/bin/bash
# Test LeviathanDM in a nested Wayland session (safe, no risk to your main session)

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Check if running in Wayland or X11
if [ -z "$WAYLAND_DISPLAY" ] && [ -z "$DISPLAY" ]; then
    echo "Error: Not running in a graphical session"
    echo "You need to be in X11 or Wayland to run nested"
    echo "Switch to TTY if you want to run directly: Ctrl+Alt+F2"
    exit 1
fi

# Determine backend based on session type
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "Detected Wayland session, using wayland backend"
    BACKEND="wayland"
elif [ -n "$DISPLAY" ]; then
    echo "Detected X11 session, using X11 backend"
    BACKEND="x11"
fi

echo "Starting LeviathanDM in nested Wayland session..."
echo "This will run in a window on your current desktop"
echo ""

# Set up environment
export LD_LIBRARY_PATH="$SCRIPT_DIR/build/wlroots-install/lib:$SCRIPT_DIR/build/libdisplay-info-install/lib"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export WLR_RENDERER=gles2
export WLR_BACKENDS="$BACKEND"  # Use detected backend

echo "Environment:"
echo "  LD_LIBRARY_PATH=$LD_LIBRARY_PATH"
echo "  WLR_BACKENDS=$WLR_BACKENDS"
echo "  XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR"
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "  WAYLAND_DISPLAY=$WAYLAND_DISPLAY"
fi
if [ -n "$DISPLAY" ]; then
    echo "  DISPLAY=$DISPLAY"
fi
echo ""

cd "$SCRIPT_DIR"
exec ./build/leviathan
