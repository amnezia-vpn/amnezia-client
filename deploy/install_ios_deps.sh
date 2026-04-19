#!/bin/bash

set -euo pipefail

YES_MODE="OFF"
for arg in "$@"; do
  case "$arg" in
    --yes|-y|--non-interactive)
      YES_MODE="ON"
      ;;
    *)
      ;;
  esac
done

if [ "$(uname -s)" != "Darwin" ]; then
  echo "This script is for macOS only."
  exit 1
fi

echo "Installing/checking iOS build dependencies..."

if [ ! -d "/Applications/Xcode.app/Contents/Developer" ]; then
  echo "Full Xcode is not installed in /Applications/Xcode.app"
  echo "Install Xcode from App Store first."
  exit 1
fi

if command -v xcode-select >/dev/null 2>&1; then
  CURRENT_DEV_DIR="$(xcode-select -p 2>/dev/null || true)"
  if [ "$CURRENT_DEV_DIR" != "/Applications/Xcode.app/Contents/Developer" ]; then
    if sudo -n true 2>/dev/null; then
      sudo xcode-select -s /Applications/Xcode.app/Contents/Developer
    else
      echo "Need full Xcode toolchain as active developer dir."
      echo "Run manually:"
      echo "  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
      exit 1
    fi
  fi
fi

if command -v xcodebuild >/dev/null 2>&1; then
  if sudo -n true 2>/dev/null; then
    sudo xcodebuild -license accept || true
  else
    echo "If Xcode license is not accepted, run:"
    echo "  sudo xcodebuild -license accept"
  fi
else
  echo "xcodebuild not found. Full Xcode install is required."
  exit 1
fi

if ! command -v brew >/dev/null 2>&1; then
  if [ "$YES_MODE" = "ON" ]; then
    echo "Homebrew is missing. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  else
    echo "Homebrew is missing."
    echo "Install it and rerun:"
    echo "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\""
    exit 1
  fi
fi

if [ -x /opt/homebrew/bin/brew ]; then
  eval "$(/opt/homebrew/bin/brew shellenv)"
elif [ -x /usr/local/bin/brew ]; then
  eval "$(/usr/local/bin/brew shellenv)"
fi

brew update
brew install cmake ninja pkg-config go || true

if ! command -v go >/dev/null 2>&1; then
  echo "Go is still not available in PATH after install."
  echo "Add brew go to PATH and rerun."
  exit 1
fi

GOBIN_PATH="${GOBIN:-$(go env GOPATH)/bin}"
export PATH="$PATH:$GOBIN_PATH"

echo "Installing gomobile..."
go install golang.org/x/mobile/cmd/gomobile@latest
"$GOBIN_PATH/gomobile" init

QT_CMAKE_CANDIDATE=""
if [ -n "${QT_BIN_DIR:-}" ] && [ -x "${QT_BIN_DIR}/qt-cmake" ]; then
  QT_CMAKE_CANDIDATE="${QT_BIN_DIR}/qt-cmake"
elif [ -x "$HOME/Qt/6.6.2/ios/bin/qt-cmake" ]; then
  QT_CMAKE_CANDIDATE="$HOME/Qt/6.6.2/ios/bin/qt-cmake"
else
  QT_CMAKE_CANDIDATE="$(ls -1d "$HOME"/Qt/*/ios/bin/qt-cmake 2>/dev/null | sort -V | tail -n1 || true)"
  if [ -z "$QT_CMAKE_CANDIDATE" ]; then
    QT_CMAKE_CANDIDATE="$(ls -1d "$HOME"/Qt/*/macos/bin/qt-cmake 2>/dev/null | sort -V | tail -n1 || true)"
  fi
fi

if [ -n "$QT_CMAKE_CANDIDATE" ] && [ -x "$QT_CMAKE_CANDIDATE" ]; then
  echo "Qt found: $QT_CMAKE_CANDIDATE"
  "$QT_CMAKE_CANDIDATE" --version || true
else
  echo "Qt iOS toolchain not found."
  echo "Install Qt iOS kit (e.g. 6.6.x) via Qt Maintenance Tool."
  echo "Expected path example: ~/Qt/6.6.2/ios/bin/qt-cmake"
fi

echo "Done."
echo "Next:"
echo "  IOS_SIMULATOR_UI_ONLY=OFF ./deploy/open_ios_xcode.sh"
