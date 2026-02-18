#!/bin/bash

EXTRACT_DIR="$1"
INSTALLER_PATH="$2"

# Create and clean extract directory
rm -rf "$EXTRACT_DIR"
mkdir -p "$EXTRACT_DIR"

# Extract TAR archive
tar -xf "$INSTALLER_PATH" -C "$EXTRACT_DIR"
if [ $? -ne 0 ]; then
    echo 'Failed to extract TAR archive'
    exit 1
fi

# Find and run installer
INSTALLER=$(find "$EXTRACT_DIR" -type f -executable)
if [ -z "$INSTALLER" ]; then
    echo 'Installer not found'
    exit 1
fi

"$INSTALLER"
EXIT_CODE=$?

# Cleanup
rm -rf "$EXTRACT_DIR"
exit $EXIT_CODE 