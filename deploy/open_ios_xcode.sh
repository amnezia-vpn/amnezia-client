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

go install golang.org/x/mobile/cmd/gomobile@latest
gomobile init

echo "Using qt-cmake: $QTCMAKE"
echo "Using QT_HOST_PATH: $QT_MACOS_ROOT_DIR"

"$QTCMAKE" . -B build-ios -GXcode -DQT_HOST_PATH="$QT_MACOS_ROOT_DIR" -DDEPLOY=ON
open build-ios/AmneziaVPN.xcodeproj

echo "Done: build-ios/AmneziaVPN.xcodeproj"
