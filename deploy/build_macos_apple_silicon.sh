#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
QT_ROOT_PATH="${QT_ROOT_PATH:-$HOME/Qt/6.10.3}"
MACOS_ARCHS="${MACOS_ARCHS:-arm64}"
BUILD_PATH="${BUILD_PATH:-$ROOT_DIR/deploy/build}"
FORCE_CONFIGURE="${FORCE_CONFIGURE:-1}"

APP_BUNDLE="$BUILD_PATH/client/AmneziaVPN.app"
APP_BINARY="$APP_BUNDLE/Contents/MacOS/AmneziaVPN"
SERVICE_BINARY="$APP_BUNDLE/Contents/MacOS/AmneziaVPN-service"

require_file() {
    local path="$1"
    if [[ ! -e "$path" ]]; then
        echo "error: missing $path" >&2
        exit 1
    fi
}

require_file "$QT_ROOT_PATH/macos/bin/macdeployqt"

cd "$ROOT_DIR"

if [[ ! -x ".venv/bin/conan" ]]; then
    python3 -m venv .venv
    .venv/bin/python -m pip install --upgrade pip conan
fi

if [[ ! -f "$HOME/.conan2/profiles/default" ]]; then
    .venv/bin/conan profile detect --force
fi

git submodule update --init --recursive

build_args=(-t macos -b "$BUILD_PATH")
if [[ "$FORCE_CONFIGURE" != "0" ]]; then
    build_args+=(-f)
fi

PATH="$ROOT_DIR/.venv/bin:$PATH" \
QT_ROOT_PATH="$QT_ROOT_PATH" \
CMAKE_OSX_ARCHITECTURES="$MACOS_ARCHS" \
    "$ROOT_DIR/deploy/build.sh" "${build_args[@]}"

"$QT_ROOT_PATH/macos/bin/macdeployqt" "$APP_BUNDLE" -always-overwrite -qmldir="$ROOT_DIR/client"

rm -f \
    "$APP_BUNDLE/Contents/PlugIns/sqldrivers/libqsqlmimer.dylib" \
    "$APP_BUNDLE/Contents/PlugIns/sqldrivers/libqsqlodbc.dylib" \
    "$APP_BUNDLE/Contents/PlugIns/sqldrivers/libqsqlpsql.dylib"

cp -p "$BUILD_PATH/service/server/AmneziaVPN-service" "$APP_BUNDLE/Contents/MacOS/"
cp -R "$ROOT_DIR/deploy/data/macos/." "$APP_BUNDLE/Contents/MacOS/"
rm -f "$APP_BUNDLE/Contents/MacOS/post_install.sh" "$APP_BUNDLE/Contents/MacOS/post_uninstall.sh"

codesign --force --deep --sign - "$APP_BUNDLE"
codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

require_file "$APP_BINARY"
require_file "$SERVICE_BINARY"

for binary in "$APP_BINARY" "$SERVICE_BINARY"; do
    archs="$(lipo -archs "$binary")"
    case " $archs " in
        *" arm64 "*) ;;
        *)
            echo "error: $binary does not contain arm64 slice: $archs" >&2
            exit 1
            ;;
    esac
    echo "$binary: $archs"
done

echo "Built: $APP_BUNDLE"
