#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "Project dir: $PROJECT_DIR"

git submodule update --init --recursive

export QT_VERSION="${QT_VERSION:-6.6.2}"
export QT_BIN_DIR="${QT_BIN_DIR:-$HOME/Qt/$QT_VERSION/ios/bin}"
export QT_MACOS_ROOT_DIR="${QT_MACOS_ROOT_DIR:-$HOME/Qt/$QT_VERSION/macos}"
export PATH="$PATH:$HOME/go/bin"

if [ ! -x "$QT_BIN_DIR/qt-cmake" ]; then
  echo "qt-cmake not found: $QT_BIN_DIR/qt-cmake"
  echo "Set QT_BIN_DIR (or QT_VERSION) and run again."
  exit 1
fi

if ! command -v go >/dev/null 2>&1; then
  echo "Go is not installed or not in PATH."
  exit 1
fi

go install golang.org/x/mobile/cmd/gomobile@latest
gomobile init

"$QT_BIN_DIR/qt-cmake" . -B build-ios -GXcode -DQT_HOST_PATH="$QT_MACOS_ROOT_DIR" -DDEPLOY=ON
open build-ios/AmneziaVPN.xcodeproj

echo "Done: build-ios/AmneziaVPN.xcodeproj"
