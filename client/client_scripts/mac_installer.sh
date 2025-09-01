#!/bin/bash

EXTRACT_DIR="$1"
INSTALLER_PATH="$2"

set -e

# Create and clean extract directory
rm -rf "$EXTRACT_DIR"
mkdir -p "$EXTRACT_DIR"

echo "[AmneziaVPN] Using extract dir: $EXTRACT_DIR"
echo "[AmneziaVPN] Installer archive: $INSTALLER_PATH"

if [ ! -f "$INSTALLER_PATH" ]; then
    echo "[AmneziaVPN] ERROR: Installer archive not found: $INSTALLER_PATH"
    exit 1
fi

# Unzip archive (ZIP expected)
echo "[AmneziaVPN] Extracting ZIP..."
ditto -x -k "$INSTALLER_PATH" "$EXTRACT_DIR"

# Locate PKG
PKG_PATH=$(find "$EXTRACT_DIR" -maxdepth 2 -type f -name 'AmneziaVPN.pkg' | head -n 1)
if [ -z "$PKG_PATH" ]; then
    echo "[AmneziaVPN] ERROR: AmneziaVPN.pkg not found after extraction"
    exit 1
fi

echo "[AmneziaVPN] Found PKG: $PKG_PATH"

# Optional: basic signature/gatekeeper checks (non-fatal)
if command -v pkgutil >/dev/null 2>&1; then
    pkgutil --check-signature "$PKG_PATH" || true
fi
if command -v spctl >/dev/null 2>&1; then
    spctl -a -vvv -t install "$PKG_PATH" || true
fi

# Run installer with admin privileges via AppleScript (prompts for password)
echo "[AmneziaVPN] Running installer..."
OSA_CMD='do shell script "/usr/sbin/installer -pkg '"$PKG_PATH"' -target /" with administrator privileges'
osascript -e "$OSA_CMD"

STATUS=$?
if [ $STATUS -ne 0 ]; then
    echo "[AmneziaVPN] ERROR: installer exited with status $STATUS"
    exit $STATUS
fi

echo "[AmneziaVPN] Cleaning up..."
rm -f "$INSTALLER_PATH" || true
rm -rf "$EXTRACT_DIR/mounted_dmg" 2>/dev/null || true

echo "[AmneziaVPN] Installation completed successfully"
exit 0
