#!/bin/bash
set -o errexit

PROJECT_DIR=$(pwd)
BUILD_DIR="$PROJECT_DIR/deploy/build"

qt_folders=()
qif_folders=()
for base in ~/Qt /opt/Qt; do
    for dir in "$base"/${QT_VERSION:-6.*}; do
        [ -d "$dir" ] && qt_folders+=("$dir")
    done
    for dir in "$base"/Tools/QtInstallerFramework/${QIF_VERSION:-*}; do
        [ -d "$dir" ] && qif_folders+=("$dir")
    done
done

: ${QT_ROOT_PATH:=$(printf '%s\n' "${qt_folders[@]}" | sort -V | tail -1)}
: ${QIF_ROOT_PATH:=$(printf '%s\n' "${qif_folders[@]}" | sort -V | tail -1)}

case "$(uname -s)" in
    Linux)
        : ${QT_PREFIX_PATH:="$QT_ROOT_PATH"/gcc_64}
        ;;
    Darwin)
        : ${QT_PREFIX_PATH:="$QT_ROOT_PATH"/macos}
        ;;
esac

args=()
if [ -n "${QT_PREFIX_PATH}" ]; then
    args+=("-DCMAKE_PREFIX_PATH=$QT_PREFIX_PATH")
fi

set -o xtrace

cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Release "${args[@]}"
cmake --build "$BUILD_DIR" --target all
(cd "$BUILD_DIR" && cpack -D QTIFWDIR="$QIF_ROOT_PATH")
