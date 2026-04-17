#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "Project dir: $PROJECT_DIR"

git submodule update --init --recursive

export QT_VERSION="${QT_VERSION:-6.6.2}"
export QT_BIN_DIR="${QT_BIN_DIR:-$HOME/Qt/$QT_VERSION/ios/bin}"
export QT_MACOS_ROOT_DIR="${QT_MACOS_ROOT_DIR:-$HOME/Qt/$QT_VERSION/macos}"
export IOS_DEPLOY="${IOS_DEPLOY:-OFF}"
export BUILD_IOS_DEVELOPMENT_TEAM="${BUILD_IOS_DEVELOPMENT_TEAM:-}"
export PATH="$PATH:$HOME/go/bin"

pick_latest_match() {
  local pattern="$1"
  local latest=""
  while IFS= read -r line; do
    latest="$line"
  done < <(ls -1d $pattern 2>/dev/null | sort -V)
  if [ -n "$latest" ]; then
    echo "$latest"
  fi
}

QTCMAKE=""
if [ -x "$QT_BIN_DIR/qt-cmake" ]; then
  QTCMAKE="$QT_BIN_DIR/qt-cmake"
elif command -v qt-cmake >/dev/null 2>&1; then
  QTCMAKE="$(command -v qt-cmake)"
else
  CANDIDATE_DIR="$(pick_latest_match "$HOME/Qt/*/ios/bin")"
  if [ -n "${CANDIDATE_DIR:-}" ] && [ -x "$CANDIDATE_DIR/qt-cmake" ]; then
    QTCMAKE="$CANDIDATE_DIR/qt-cmake"
    QT_BIN_DIR="$CANDIDATE_DIR"
  else
    CANDIDATE_DIR="$(pick_latest_match "$HOME/Qt/*/macos/bin")"
    if [ -n "${CANDIDATE_DIR:-}" ] && [ -x "$CANDIDATE_DIR/qt-cmake" ]; then
      QTCMAKE="$CANDIDATE_DIR/qt-cmake"
      QT_BIN_DIR="$CANDIDATE_DIR"
    fi
  fi
fi

if [ -z "$QTCMAKE" ] || [ ! -x "$QTCMAKE" ]; then
  echo "qt-cmake not found."
  echo "Expected one of:"
  echo "  \$QT_BIN_DIR/qt-cmake"
  echo "  ~/Qt/<version>/ios/bin/qt-cmake"
  echo "  ~/Qt/<version>/macos/bin/qt-cmake"
  echo "Set QT_BIN_DIR (or QT_VERSION) and run again."
  exit 1
fi

if [ ! -d "$QT_MACOS_ROOT_DIR" ]; then
  QTCMAKE_DIR="$(cd "$(dirname "$QTCMAKE")" && pwd)"
  case "$QTCMAKE_DIR" in
    */ios/bin)
      QT_BASE_DIR="$(cd "$QTCMAKE_DIR/../.." && pwd)"
      QT_MACOS_ROOT_DIR="$QT_BASE_DIR/macos"
      ;;
    */macos/bin)
      QT_MACOS_ROOT_DIR="$(cd "$QTCMAKE_DIR/.." && pwd)"
      ;;
    *)
      QT_MACOS_ROOT_DIR="$(cd "$QTCMAKE_DIR/.." && pwd)"
      ;;
  esac
fi

if ! command -v go >/dev/null 2>&1; then
  echo "Go is not installed or not in PATH."
  exit 1
fi

# Prefer full Xcode over CommandLineTools for -GXcode generator.
if [ -z "${DEVELOPER_DIR:-}" ]; then
  if [ -d "/Applications/Xcode.app/Contents/Developer" ]; then
    export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"
  else
    XCODE_APP_CANDIDATE="$(ls -1d /Applications/Xcode*.app 2>/dev/null | sort -V | tail -n1 || true)"
    if [ -n "$XCODE_APP_CANDIDATE" ] && [ -d "$XCODE_APP_CANDIDATE/Contents/Developer" ]; then
      export DEVELOPER_DIR="$XCODE_APP_CANDIDATE/Contents/Developer"
    fi
  fi
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
  echo "xcodebuild not found. Install full Xcode from App Store."
  exit 1
fi

XCODE_VERSION_LINE="$(xcodebuild -version 2>/dev/null | head -n1 || true)"
if [ -z "$XCODE_VERSION_LINE" ]; then
  echo "Cannot read Xcode version."
  echo "Most likely: active developer dir points to CommandLineTools and full Xcode is not installed."
  echo "Install full Xcode (App Store) and run:"
  echo "  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
  echo "  sudo xcodebuild -license accept"
  exit 1
fi

XCODE_VERSION="$(echo "$XCODE_VERSION_LINE" | awk '{print $2}')"
XCODE_MAJOR="$(echo "$XCODE_VERSION" | cut -d. -f1)"
if [ -z "$XCODE_MAJOR" ] || [ "$XCODE_MAJOR" -lt 12 ]; then
  echo "Unsupported Xcode version: $XCODE_VERSION_LINE"
  echo "Install/update full Xcode (12+), then run:"
  echo "  sudo xcode-select -s /Applications/Xcode.app/Contents/Developer"
  exit 1
fi

go install golang.org/x/mobile/cmd/gomobile@latest
gomobile init

echo "Using qt-cmake: $QTCMAKE"
echo "Using QT_HOST_PATH: $QT_MACOS_ROOT_DIR"
echo "Using DEVELOPER_DIR: ${DEVELOPER_DIR:-$(xcode-select -p 2>/dev/null || echo 'not set')}"
echo "Detected Xcode: $XCODE_VERSION_LINE"
echo "Using IOS_DEPLOY: $IOS_DEPLOY"
if [ -n "$BUILD_IOS_DEVELOPMENT_TEAM" ]; then
  echo "Using BUILD_IOS_DEVELOPMENT_TEAM: $BUILD_IOS_DEVELOPMENT_TEAM"
fi

CMAKE_ARGS=(
  .
  -B build-ios
  -GXcode
  -DQT_HOST_PATH="$QT_MACOS_ROOT_DIR"
  -DCMAKE_OSX_SYSROOT=iphoneos
  -DCMAKE_OSX_ARCHITECTURES=arm64
)

if [ "$IOS_DEPLOY" = "ON" ]; then
  CMAKE_ARGS+=(-DDEPLOY=ON)
fi

if [ -n "$BUILD_IOS_DEVELOPMENT_TEAM" ]; then
  CMAKE_ARGS+=(-DBUILD_IOS_DEVELOPMENT_TEAM="$BUILD_IOS_DEVELOPMENT_TEAM")
fi

"$QTCMAKE" "${CMAKE_ARGS[@]}"

XCODEPROJ_PATH=""
if [ -d "build-ios/AmneziaVPN.xcodeproj" ]; then
  XCODEPROJ_PATH="build-ios/AmneziaVPN.xcodeproj"
elif [ -d "build-ios/FBLink.xcodeproj" ]; then
  XCODEPROJ_PATH="build-ios/FBLink.xcodeproj"
else
  XCODEPROJ_PATH="$(ls -1d build-ios/*.xcodeproj 2>/dev/null | head -n1 || true)"
fi

if [ -z "$XCODEPROJ_PATH" ] || [ ! -d "$XCODEPROJ_PATH" ]; then
  echo "Failed to locate generated .xcodeproj under build-ios/"
  exit 1
fi

open "$XCODEPROJ_PATH"

echo "Done: $XCODEPROJ_PATH"
