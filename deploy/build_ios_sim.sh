#!/bin/bash

set -euo pipefail

if [ "$(uname -s)" != "Darwin" ]; then
  echo "This script is for macOS only."
  exit 1
fi

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

QT_VERSION="${QT_VERSION:-6.6.2}"
QT_BIN_DIR="${QT_BIN_DIR:-$HOME/Qt/$QT_VERSION/ios/bin}"
QT_MACOS_ROOT_DIR="${QT_MACOS_ROOT_DIR:-$HOME/Qt/$QT_VERSION/macos}"

BUILD_IOS_DIR="${BUILD_IOS_DIR:-build-ios-sim}"
BUILD_CONFIG="${BUILD_CONFIG:-Debug}"
BUILD_TARGET="${BUILD_TARGET:-FBLink}"
CLEAN_BUILD="${CLEAN_BUILD:-ON}"
CLEAN_DERIVED_DATA="${CLEAN_DERIVED_DATA:-ON}"

if [ -n "${IOS_ARCH:-}" ]; then
  SIM_ARCH="$IOS_ARCH"
elif [ "$(uname -m)" = "arm64" ]; then
  SIM_ARCH="arm64"
else
  SIM_ARCH="x86_64"
fi

if [ "$CLEAN_BUILD" = "ON" ]; then
  echo "Cleaning build dir: $BUILD_IOS_DIR"
  rm -rf "$BUILD_IOS_DIR"
fi

if [ "$CLEAN_DERIVED_DATA" = "ON" ]; then
  echo "Cleaning Xcode DerivedData (FBLink*)"
  rm -rf "$HOME/Library/Developer/Xcode/DerivedData/FBLink"*
fi

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_IOS_DIR"
echo "Simulator arch: $SIM_ARCH"

OPEN_XCODE=OFF \
IOS_DEPLOY=OFF \
IOS_SIMULATOR_UI_ONLY=ON \
BUILD_IOS_NETWORK_EXTENSION=OFF \
IOS_ARCH="$SIM_ARCH" \
QT_VERSION="$QT_VERSION" \
QT_BIN_DIR="$QT_BIN_DIR" \
QT_MACOS_ROOT_DIR="$QT_MACOS_ROOT_DIR" \
BUILD_IOS_DIR="$BUILD_IOS_DIR" \
bash "$PROJECT_DIR/deploy/open_ios_xcode.sh"

cmake --build "$BUILD_IOS_DIR" --config "$BUILD_CONFIG" --target "$BUILD_TARGET"

echo "Done. iOS simulator build succeeded."
