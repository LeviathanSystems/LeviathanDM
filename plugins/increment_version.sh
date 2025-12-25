#!/bin/bash
# Auto-increment plugin build number
# Usage: ./increment_version.sh <version_file>

VERSION_FILE="$1"

if [ ! -f "$VERSION_FILE" ]; then
    echo "1.0.0.1" > "$VERSION_FILE"
    echo "1.0.0.1"
    exit 0
fi

# Read current version
VERSION=$(cat "$VERSION_FILE")

# Parse version components (major.minor.patch.build)
IFS='.' read -r MAJOR MINOR PATCH BUILD <<< "$VERSION"

# Increment build number
BUILD=$((BUILD + 1))

# Write new version
NEW_VERSION="${MAJOR}.${MINOR}.${PATCH}.${BUILD}"
echo "$NEW_VERSION" > "$VERSION_FILE"
echo "$NEW_VERSION"
