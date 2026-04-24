#!/bin/bash
set -o errexit
set +o xtrace

run_traced() {
    PS4='\033[1;34m+ \033[0m'
    set -o xtrace
    "$@"
    { set +o xtrace; } 2>/dev/null
}

PROJECT_DIR=$(pwd)
BUILD_DIR="$PROJECT_DIR/deploy/build"

installers=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--target)        TARGET="$2";         shift 2 ;;
        -f|--force)         FORCE=true;          shift ;;
        -i|--installer)     installers+=("$2")   shift 2 ;;
        -k|--keychain)      KEYCHAIN="$2"        shift 2 ;;
        --qt-install-dir)   QT_INSTALL_DIR="$2"; shift 2 ;;
        --help|-h|?)
            echo "Usage: $0 [options] 
Options:
    -t|--target <name>        - specify build target
    -f|--force                - force removal of build folder prior cmake configuration
    -i|--installer <name|all> - specify an installer to build. allowed to be used multiple times
    --qt-install-dir <path>   - specify Qt install path to be used during building
            "
            exit 0
            ;;
            *) echo "Unknown arg"; exit 1 ;;
    esac
done

: ${INSTALLERS:="${installers[@]}"}

bases=(~/Qt /opt/Qt)
[ -n "${QT_INSTALL_DIR}" ] && bases=("${QT_INSTALL_DIR}/Qt" "${bases[@]}")

qt_folders=()
qif_folders=()
for base in "${bases[@]}"; do
    for dir in "$base"/${QT_VERSION:-6.*}; do
        [ -d "$dir" ] && qt_folders+=("$dir")
    done
    for dir in "$base"/Tools/QtInstallerFramework/${QIF_VERSION:-*}; do
        [ -d "$dir" ] && qif_folders+=("$dir")
    done
done

: ${QT_ROOT_PATH:=$(printf '%s\n' "${qt_folders[@]}" | sort -V | tail -1)}
: ${QIF_ROOT_PATH:=$(printf '%s\n' "${qif_folders[@]}" | sort -V | tail -1)}

: ${TARGET:="$(uname -s)"}
case "$TARGET" in
    Linux|linux)
        [ "$INSTALLERS" = "all" ] && INSTALLERS="IFW"
        : ${CMAKE_GENERATOR:="Unix Makefiles"}
        [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_PREFIX_PATH:="$QT_ROOT_PATH"/gcc_64}
        ;;
    Darwin|darwin|MacOS|macos)
        [ "$INSTALLERS" = "all" ] && INSTALLERS="IFW"
        : ${CMAKE_GENERATOR:="Xcode"}
        [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_PREFIX_PATH:="$QT_ROOT_PATH"/macos}
        ;;
    macos-ne)
        MACOS_NE=TRUE
        no_installers=1
        : ${CMAKE_GENERATOR:="Xcode"}
        [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_PREFIX_PATH:="$QT_ROOT_PATH"/macos}
        ;;
    iOS|ios)
        no_installers=1
        : ${CMAKE_GENERATOR:="Xcode"}
        if [[ -n "${QT_ROOT_PATH}" ]]; then
            : ${QT_HOST_PATH:="$QT_ROOT_PATH/macos"}
            : ${CMAKE_TOOLCHAIN_FILE:="$QT_ROOT_PATH/ios/lib/cmake/Qt6/qt.toolchain.cmake"}
        fi
        : ${CMAKE_OSX_SYSROOT=iphoneos}
        ;;
esac

args=()
[[ -n "${CMAKE_GENERATOR}" ]]      && args+=("-G" "$CMAKE_GENERATOR")
[[ -n "${QT_PREFIX_PATH}" ]]       && args+=("-DCMAKE_PREFIX_PATH=$QT_PREFIX_PATH")
[[ -n "${CMAKE_TOOLCHAIN_FILE}" ]] && args+=("-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE")
[[ -n "${QT_HOST_PATH}" ]]         && args+=("-DQT_HOST_PATH=$QT_HOST_PATH")
[[ -n "${CMAKE_OSX_SYSROOT}" ]]    && args+=("-DCMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT")
[[ -n "${MACOS_NE}" ]]             && args+=("-DMACOS_NE=$MACOS_NE")
[[ -n "${KEYCHAIN}" ]]             && args+=("-DBUILD_VPN_KEYCHAIN=$KEYCHAIN")

: ${CMAKE_BUILD_TYPE:=Release}

[[ -n "$FORCE" ]] && run_traced rm -rf "$BUILD_DIR"
run_traced cmake -S "$PROJECT_DIR" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" "${args[@]}"
run_traced cmake --build "$BUILD_DIR" --config "$CMAKE_BUILD_TYPE"

if [ -z "$no_installers" ]; then
    for installer in "$INSTALLERS"; do
        (cd "$BUILD_DIR" && run_traced cpack -G "$installer" -D QTIFWDIR="$QIF_ROOT_PATH")
    done
fi
