#!/bin/bash
# Wrapper script to run LeviathanDM with proper library paths

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
SPDLOG_DIR="${BUILD_DIR}/_deps/spdlog-build"

# Set LD_LIBRARY_PATH to include our build directories
export LD_LIBRARY_PATH="${BUILD_DIR}:${SPDLOG_DIR}${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Run leviathan
exec "${BUILD_DIR}/leviathan" "$@"
