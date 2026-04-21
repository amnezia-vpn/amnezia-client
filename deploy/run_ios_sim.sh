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
BUNDLE_ID="${BUNDLE_ID:-}"
SIMULATOR_NAME="${SIMULATOR_NAME:-iPhone 16 Pro}"
DEVICE_SELECTOR="${DEVICE_SELECTOR:-}"
CLEAN_INSTALL="${CLEAN_INSTALL:-ON}"

APP_PATH="$PROJECT_DIR/$BUILD_IOS_DIR/client/${BUILD_CONFIG}-iphonesimulator/${APP_NAME}.app"
if [ ! -d "$APP_PATH" ]; then
  echo "App bundle not found: $APP_PATH"
  echo "Build first, for example:"
  echo "  QT_VERSION=6.11.0 ./deploy/build_ios_sim.sh"
  exit 1
fi

APP_BUNDLE_ID=""
if [ -f "$APP_PATH/Info.plist" ]; then
  APP_BUNDLE_ID="$(/usr/libexec/PlistBuddy -c "Print :CFBundleIdentifier" "$APP_PATH/Info.plist" 2>/dev/null || true)"
fi

if [ -z "$BUNDLE_ID" ]; then
  BUNDLE_ID="$APP_BUNDLE_ID"
fi

if [ -z "$BUNDLE_ID" ]; then
  echo "Cannot determine bundle identifier. Set BUNDLE_ID explicitly."
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
if [ "$CLEAN_INSTALL" = "ON" ]; then
  xcrun simctl uninstall "$DEVICE_SELECTOR" "$BUNDLE_ID" >/dev/null 2>&1 || true
fi
xcrun simctl install "$DEVICE_SELECTOR" "$APP_PATH"

echo "Launching bundle: $BUNDLE_ID"
set +e
xcrun simctl launch --console "$DEVICE_SELECTOR" "$BUNDLE_ID"
LAUNCH_RC=$?
set -e

if [ "$LAUNCH_RC" -ne 0 ]; then
  echo
  echo "Launch failed with code $LAUNCH_RC."
  echo "App bundle id in Info.plist: ${APP_BUNDLE_ID:-unknown}"
  echo "Bundle id used for launch: $BUNDLE_ID"
  echo
  echo "Recent simulator logs:"
  xcrun simctl spawn "$DEVICE_SELECTOR" log show --style compact --last 3m --predicate "process CONTAINS[c] \"$APP_NAME\" OR eventMessage CONTAINS[c] \"$BUNDLE_ID\"" | tail -n 200 || true
  echo
  echo "Try:"
  echo "  xcrun simctl uninstall $DEVICE_SELECTOR $BUNDLE_ID"
  echo "  xcrun simctl install $DEVICE_SELECTOR \"$APP_PATH\""
  echo "  xcrun simctl launch --console $DEVICE_SELECTOR $BUNDLE_ID"
  exit "$LAUNCH_RC"
fi

echo "App launched."
