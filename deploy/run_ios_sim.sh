#!/bin/bash

set -euo pipefail

if [ "$(uname -s)" != "Darwin" ]; then
  echo "This script is for macOS only."
  exit 1
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

BUILD_IOS_DIR="${BUILD_IOS_DIR:-build-ios-sim}"
BUILD_CONFIG="${BUILD_CONFIG:-Debug}"
APP_NAME="${APP_NAME:-FBLink}"
BUNDLE_ID="${BUNDLE_ID:-com.fblink.vpn}"
SIMULATOR_NAME="${SIMULATOR_NAME:-iPhone 16 Pro}"
DEVICE_SELECTOR="${DEVICE_SELECTOR:-}"

APP_PATH="$PROJECT_DIR/$BUILD_IOS_DIR/client/${BUILD_CONFIG}-iphonesimulator/${APP_NAME}.app"
if [ ! -d "$APP_PATH" ]; then
  echo "App bundle not found: $APP_PATH"
  echo "Build first, for example:"
  echo "  QT_VERSION=6.11.0 ./deploy/build_ios_sim.sh"
  exit 1
fi

if [ -z "$DEVICE_SELECTOR" ]; then
  if xcrun simctl list devices | grep -q "(Booted)"; then
    DEVICE_SELECTOR="booted"
  else
    echo "Booting simulator: $SIMULATOR_NAME"
    xcrun simctl boot "$SIMULATOR_NAME" >/dev/null 2>&1 || true
    open -a Simulator >/dev/null 2>&1 || true
    DEVICE_SELECTOR="booted"
  fi
fi

echo "Installing: $APP_PATH"
xcrun simctl install "$DEVICE_SELECTOR" "$APP_PATH"

echo "Launching bundle: $BUNDLE_ID"
set +e
xcrun simctl launch --console "$DEVICE_SELECTOR" "$BUNDLE_ID"
LAUNCH_RC=$?
set -e

if [ "$LAUNCH_RC" -ne 0 ]; then
  echo
  echo "Launch failed with code $LAUNCH_RC."
  echo "Try:"
  echo "  xcrun simctl uninstall $DEVICE_SELECTOR $BUNDLE_ID"
  echo "  xcrun simctl install $DEVICE_SELECTOR \"$APP_PATH\""
  echo "  xcrun simctl launch --console $DEVICE_SELECTOR $BUNDLE_ID"
  exit "$LAUNCH_RC"
fi

echo "App launched."
