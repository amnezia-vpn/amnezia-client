#!/bin/bash

set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

echo "Project dir: $PROJECT_DIR"

if [ "${INSTALL_IOS_DEPS:-OFF}" = "ON" ]; then
  bash "$PROJECT_DIR/deploy/install_ios_deps.sh" --yes
fi

git submodule update --init --recursive

export QT_VERSION="${QT_VERSION:-6.6.2}"
export QT_BIN_DIR="${QT_BIN_DIR:-$HOME/Qt/$QT_VERSION/ios/bin}"
export QT_MACOS_ROOT_DIR="${QT_MACOS_ROOT_DIR:-$HOME/Qt/$QT_VERSION/macos}"
export REQUIRED_QT_VERSION_PREFIX="${REQUIRED_QT_VERSION_PREFIX:-6.6.2}"
export ENFORCE_QT_VERSION="${ENFORCE_QT_VERSION:-OFF}"
export IOS_DEPLOY="${IOS_DEPLOY:-OFF}"
export OPEN_XCODE="${OPEN_XCODE:-ON}"
if [ -z "${IOS_SIMULATOR_UI_ONLY+x}" ]; then
  IOS_SIMULATOR_UI_ONLY="OFF"
fi
export IOS_SIMULATOR_UI_ONLY
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

QT_EFFECTIVE_VERSION="$("$QTCMAKE" --version 2>/dev/null | awk '/Using Qt version/{print $4; exit}')"
if [ -z "$QT_EFFECTIVE_VERSION" ]; then
  QT_EFFECTIVE_VERSION="$(echo "$QTCMAKE" | sed -nE 's@.*/Qt/([^/]+)/.*@\1@p')"
fi

if [ -n "$REQUIRED_QT_VERSION_PREFIX" ]; then
  case "$QT_EFFECTIVE_VERSION" in
    ${REQUIRED_QT_VERSION_PREFIX}*) ;;
    *)
      if [ "$ENFORCE_QT_VERSION" = "ON" ]; then
        echo "Detected Qt version: ${QT_EFFECTIVE_VERSION:-unknown}"
        echo "This project expects Qt ${REQUIRED_QT_VERSION_PREFIX} for iOS simulator builds."
        echo "Set QT_BIN_DIR/QT_MACOS_ROOT_DIR to ${REQUIRED_QT_VERSION_PREFIX} kit and rerun."
        exit 1
      else
        echo "WARNING: Detected Qt version: ${QT_EFFECTIVE_VERSION:-unknown}"
        echo "WARNING: Recommended version for this project is ${REQUIRED_QT_VERSION_PREFIX}."
      fi
      ;;
  esac
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
echo "Using Qt version: ${QT_EFFECTIVE_VERSION:-unknown}"
echo "Using QT_HOST_PATH: $QT_MACOS_ROOT_DIR"
echo "Using DEVELOPER_DIR: ${DEVELOPER_DIR:-$(xcode-select -p 2>/dev/null || echo 'not set')}"
echo "Detected Xcode: $XCODE_VERSION_LINE"
echo "Using IOS_DEPLOY: $IOS_DEPLOY"
echo "Using IOS_SIMULATOR_UI_ONLY: $IOS_SIMULATOR_UI_ONLY"
if [ -n "$BUILD_IOS_DEVELOPMENT_TEAM" ]; then
  echo "Using BUILD_IOS_DEVELOPMENT_TEAM: $BUILD_IOS_DEVELOPMENT_TEAM"
fi

IOS_SYSROOT="iphoneos"
if [ -n "${IOS_ARCH:-}" ]; then
  IOS_ARCH_VALUE="$IOS_ARCH"
else
  IOS_ARCH_VALUE="arm64"
fi
IOS_NE="${BUILD_IOS_NETWORK_EXTENSION:-ON}"
if [ "$IOS_SIMULATOR_UI_ONLY" = "ON" ]; then
  IOS_SYSROOT="iphonesimulator"
  if [ -z "${IOS_ARCH:-}" ]; then
    if [ "$(uname -m)" = "arm64" ]; then
      IOS_ARCH_VALUE="arm64"
    else
      IOS_ARCH_VALUE="x86_64"
    fi
  fi
  if [ -z "${BUILD_IOS_NETWORK_EXTENSION+x}" ]; then
    IOS_NE="OFF"
  fi
  echo "WARNING: IOS_SIMULATOR_UI_ONLY=ON selected."
  echo "WARNING: Full VPN build may fail on simulator if simulator prebuilt libs are missing."
fi

if [ -z "${BUILD_IOS_DIR+x}" ]; then
  if [ "$IOS_SIMULATOR_UI_ONLY" = "ON" ]; then
    BUILD_IOS_DIR="build-ios-sim"
  else
    BUILD_IOS_DIR="build-ios"
  fi
fi

CMAKE_ARGS=(
  .
  -B "$BUILD_IOS_DIR"
  -GXcode
  -DQT_HOST_PATH="$QT_MACOS_ROOT_DIR"
  -DCMAKE_OSX_SYSROOT="$IOS_SYSROOT"
  -DCMAKE_OSX_ARCHITECTURES="$IOS_ARCH_VALUE"
  -DBUILD_IOS_NETWORK_EXTENSION="$IOS_NE"
  -DIOS_SIMULATOR_UI_ONLY="$IOS_SIMULATOR_UI_ONLY"
)

if [ "$IOS_DEPLOY" = "ON" ]; then
  CMAKE_ARGS+=(-DDEPLOY=ON)
fi

if [ -n "$BUILD_IOS_DEVELOPMENT_TEAM" ]; then
  CMAKE_ARGS+=(-DBUILD_IOS_DEVELOPMENT_TEAM="$BUILD_IOS_DEVELOPMENT_TEAM")
fi

"$QTCMAKE" "${CMAKE_ARGS[@]}"

XCODEPROJ_PATH=""
if [ -d "$BUILD_IOS_DIR/AmneziaVPN.xcodeproj" ]; then
  XCODEPROJ_PATH="$BUILD_IOS_DIR/AmneziaVPN.xcodeproj"
elif [ -d "$BUILD_IOS_DIR/FBLink.xcodeproj" ]; then
  XCODEPROJ_PATH="$BUILD_IOS_DIR/FBLink.xcodeproj"
else
  XCODEPROJ_PATH="$(ls -1d "$BUILD_IOS_DIR"/*.xcodeproj 2>/dev/null | head -n1 || true)"
fi

if [ -z "$XCODEPROJ_PATH" ] || [ ! -d "$XCODEPROJ_PATH" ]; then
  echo "Failed to locate generated .xcodeproj under $BUILD_IOS_DIR/"
  exit 1
fi

if [ "$OPEN_XCODE" = "ON" ]; then
  open "$XCODEPROJ_PATH"
  echo "Done: $XCODEPROJ_PATH"
else
  echo "Configured: $XCODEPROJ_PATH"
fi
