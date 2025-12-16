#!/bin/bash
# Helper script to run applications in LeviathanDM compositor

export WAYLAND_DISPLAY=wayland-2
exec "$@"
