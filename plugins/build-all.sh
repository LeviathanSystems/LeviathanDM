#!/bin/bash
# Build all plugins

set -e

echo "Building all LeviathanDM plugins..."
echo ""

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SUCCESS_COUNT=0
FAIL_COUNT=0

# Find all plugin directories (those with CMakeLists.txt or build.sh)
for plugin_dir in "$SCRIPT_DIR"/*/; do
    plugin_name=$(basename "$plugin_dir")
    
    # Skip if not a directory or if it's the build guide
    if [ ! -d "$plugin_dir" ] || [ "$plugin_name" = "BUILD_GUIDE.md" ]; then
        continue
    fi
    
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "Building: $plugin_name"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    
    cd "$plugin_dir"
    
    # Try to build using build.sh if it exists
    if [ -x "build.sh" ]; then
        if ./build.sh; then
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "✗ Failed to build $plugin_name"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
    # Otherwise try CMake
    elif [ -f "CMakeLists.txt" ]; then
        mkdir -p build
        cd build
        
        if cmake .. && make; then
            echo "✓ Built $plugin_name successfully"
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            echo "✗ Failed to build $plugin_name"
            FAIL_COUNT=$((FAIL_COUNT + 1))
        fi
        
        cd ..
    else
        echo "⊘ No build system found for $plugin_name"
    fi
    
    echo ""
done

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Build Summary"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "Success: $SUCCESS_COUNT"
echo "Failed:  $FAIL_COUNT"
echo ""

if [ $FAIL_COUNT -eq 0 ]; then
    echo "✓ All plugins built successfully!"
    exit 0
else
    echo "✗ Some plugins failed to build"
    exit 1
fi
