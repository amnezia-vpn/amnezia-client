#!/bin/bash
set -o errexit
set +o xtrace

run_traced() {
    PS4='\033[1;34m+ \033[0m'
    set -o xtrace
    "$@"
    { set +o xtrace; } 2>/dev/null
}

all_abis_set="arm64-v8a armeabi-v7a x86_64 x86"
get_abi_folder() {
    case $1 in
        arm64-v8a)   echo "android_arm64_v8a" ;;
        armeabi-v7a) echo "android_armv7"     ;;
        x86)         echo "android_x86"       ;;
        all|x86_64)  echo "android_x86_64"    ;;
        *)           echo ""                  ;;
    esac
}

abis=()
installers=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -b|--build)         : ${BUILD_PATH:="$2"};   shift 2 ;;
        -s|--source)        : ${SOURCE_PATH:="$2"};  shift 2 ;;
        -t|--target)        TARGET="$2";             shift 2 ;;
        -f|--force)         : ${FORCE=true};         shift   ;;
        -g|--generator)     : ${CMAKE_GENERATOR=$2}; shift 2 ;;
        --installer)        installers+=("$2");      shift 2 ;;
        --abi)              abis+=("$2");            shift 2 ;;
        --sign)             : ${SIGN:=true};         shift   ;;
        --aab)              : ${BUILD_AAB=true};     shift   ;;
        --help|-h|?)
            echo "Usage: $0 [options]"
            echo "  Options:"
            echo "  -b|--build <path>         - specify build folder"
            echo "  -s|--source <path>        - specify path to amnezia-client root folder"
            echo "  -t|--target <name>        - specify build target"
            echo "  -f|--force                - force removal of build folder prior cmake configuration"
            echo "  -g|--generator <name>     - use specified generator for CMake"
            echo "  --installer <name|all>    - specify an installer(s) to build. allowed to be used multiple times"
            echo "  --abi                     - specify Android ABIs for target to build for. all by default"
            echo "  --sign                    - whether to sign the resulting files. only appicable to Android"
            echo "  --aab                     - whether to build AAB. only applicable to Android"
            exit 0
            ;;
        *) echo "Unknown arg \"$1\". Use $0 -h to get help"; exit 1 ;;
    esac
done

: ${SOURCE_PATH:=$(pwd)}
: ${BUILD_PATH:="$SOURCE_PATH/deploy/build"}
: ${INSTALLERS:="${installers[@]}"}
: ${ABIS:="${abis[@]}"}
: ${ABIS:="all"}
: ${HOST:="$(uname -s)"}
: ${TARGET:="$HOST"}

HOST=$(echo "$HOST" | tr '[:upper:]' '[:lower:]')
TARGET=$(echo "$TARGET" | tr '[:upper:]' '[:lower:]')

bases=(~/Qt /opt/Qt)
[ -n "${QT_INSTALL_DIR}" ] && bases=("${QT_INSTALL_DIR}/Qt" "${bases[@]}")

# seek for Qt installation in bases folders
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

: ${QT_ROOT_PATH:=$(printf '%s\n' "${qt_folders[@]}" | awk -F'/' '{print $NF, $0}' | sort -V | tail -1 | awk '{print $2}')}
: ${QIF_ROOT_PATH:=$(printf '%s\n' "${qif_folders[@]}" | awk -F'/' '{print $NF, $0}' | sort -V | tail -1 | awk '{print $2}')}

if [[ -z "$QT_ROOT_PATH" ]]; then
    echo "* Qt not found in standard paths and in QT_INSTALL_DIR"
    echo "  Please install the suitable version of Qt"
    echo "  or specify it by using QT_ROOT_PATH/QT_INSTALL_DIR variables"
    exit 1
fi

# add host options
case "$HOST" in
    linux)  [[ "$HOST" != "$TARGET" ]] && [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_HOST_PATH:="$QT_ROOT_PATH/gcc_64"} ;;
    darwin) [[ "$HOST" != "$TARGET" ]] && [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_HOST_PATH:="$QT_ROOT_PATH/macos"} ;;
    *) echo "Unsupported host \"$HOST\""; exit 1 ;;
esac

# add custom per-target options
case "$TARGET" in
    linux)
        [ "$INSTALLERS" = "all" ] && INSTALLERS="IFW"
        : ${CMAKE_GENERATOR:="Unix Makefiles"}
        : ${CMAKE_PREFIX_PATH:="$QT_ROOT_PATH"/gcc_64}
        ;;
    darwin|macos)
        [ "$INSTALLERS" = "all" ] && INSTALLERS="productbuild"
        : ${CMAKE_GENERATOR:="Unix Makefiles"}
        : ${CMAKE_PREFIX_PATH:="$QT_ROOT_PATH"/macos}
        ;;
    macos-ne)
        MACOS_NE=TRUE
        DEPLOY=1
        no_installers=1
        : ${CMAKE_GENERATOR:="Xcode"}
        : ${CMAKE_PREFIX_PATH:="$QT_ROOT_PATH"/macos}
        ;;
    ios)
        DEPLOY=1
        no_installers=1
        : ${CMAKE_GENERATOR:="Xcode"}
        : ${CMAKE_OSX_SYSROOT=iphoneos}
        : ${CMAKE_TOOLCHAIN_FILE:="$QT_ROOT_PATH/ios/lib/cmake/Qt6/qt.toolchain.cmake"}
        ;;
    android)
        no_installers=1
        : ${CMAKE_GENERATOR:="Ninja"}
        : ${ANDROID_PLATFORM:="android-28"}

        if [[ -n "$SIGN" ]]; then
            QT_ANDROID_SIGN_APK=TRUE
            QT_ANDROID_SIGN_AAB=TRUE
        fi

        [[ "$ABIS" == "all" ]] && ABIS="$all_abis_set"

        toolchain_abi=""
        for abi in $ABIS; do
            abi_exists=$(get_abi_folder "$abi")
            if [[ -z "$abi_exists" ]]; then
                echo "Unsupported ABI \"${abi}\""
                exit 1
            fi
            : ${toolchain_abi:="$abi"}
        done

        if [[ "$ABIS" == "$all_abis_set" ]]; then
            QT_ANDROID_BUILD_ALL_ABIS=TRUE
        else
            QT_ANDROID_ABIS="${ABIS// /;}"
        fi

        toolchain_dir=$(get_abi_folder "$toolchain_abi")
        : ${CMAKE_PREFIX_PATH:="$QT_ROOT_PATH/$toolchain_dir/lib/cmake/Qt6/qt.toolchain.cmake"}
        : ${CMAKE_TOOLCHAIN_FILE:="$QT_ROOT_PATH/$toolchain_dir/lib/cmake/Qt6/qt.toolchain.cmake"}
        ;;
    *) echo "Unsupported target \"$TARGET\""; exit 1 ;;
esac

if [[ "$INSTALLERS" =~ IFW ]] && [[ -z "$QIF_ROOT_PATH" ]]; then
    echo "* Qt Installer Framework not found in standard paths and in QT_INSTALL_DIR"
    echo "  Please install the suitable version of Qt Installer Framework"
    echo "  or specify it by using QIF_ROOT_PATH/QT_INSTALL_DIR variables"
    exit 1
fi

# search for Android SDK and NDK
if [[ "$TARGET" == "android" ]]; then
    bases=()
    case "$HOST" in
        linux)  bases=(~/Android/sdk)         ;;
        darwin) bases=(~/Library/Android/sdk) ;;
    esac
    [[ -n "$ANDROID_HOME" ]] && bases=("$ANDROID_HOME" "${bases[@]}")

    ndk_dirs=()
    for base in "${bases[@]}"; do
        for ndk_dir in "$base"/ndk/${ANDROID_NDK_VERSION:-*}; do
            [[ -d "$ndk_dir" ]] && ndk_dirs+=("$ndk_dir")
        done
    done

    : ${ANDROID_NDK_ROOT:=$(printf '%s\n' "${ndk_dirs[@]}" | awk -F'/' '{print $NF, $0}' | sort -V | tail -1 | awk '{print $2}')}
    : ${ANDROID_SDK_ROOT:="$ANDROID_NDK_ROOT/../.."}
fi

: ${CMAKE_BUILD_TYPE:=Release}

args=()
[[ -n "$CMAKE_GENERATOR" ]]           && args+=("-G" "$CMAKE_GENERATOR")
[[ -n "$CMAKE_BUILD_TYPE" ]]          && args+=("-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE")
[[ -n "$CMAKE_PREFIX_PATH" ]]         && args+=("-DCMAKE_PREFIX_PATH=$CMAKE_PREFIX_PATH")
[[ -n "$CMAKE_TOOLCHAIN_FILE" ]]      && args+=("-DCMAKE_TOOLCHAIN_FILE=$CMAKE_TOOLCHAIN_FILE")
[[ -n "$QT_HOST_PATH" ]]              && args+=("-DQT_HOST_PATH=$QT_HOST_PATH")
[[ -n "$CMAKE_OSX_SYSROOT" ]]         && args+=("-DCMAKE_OSX_SYSROOT=$CMAKE_OSX_SYSROOT")
[[ -n "$MACOS_NE" ]]                  && args+=("-DMACOS_NE=$MACOS_NE")
[[ -n "$DEPLOY" ]]                    && args+=("-DDEPLOY=$DEPLOY")
[[ -n "$ANDROID_ABI" ]]               && args+=("-DANDROID_ABI=$ANDROID_ABI")
[[ -n "$ANDROID_SDK_ROOT" ]]          && args+=("-DANDROID_SDK_ROOT=$ANDROID_SDK_ROOT")
[[ -n "$ANDROID_NDK_ROOT" ]]          && args+=("-DANDROID_NDK_ROOT=$ANDROID_NDK_ROOT")
[[ -n "$ANDROID_PLATFORM" ]]          && args+=("-DANDROID_PLATFORM=$ANDROID_PLATFORM")
[[ -n "$QT_ANDROID_SIGN_APK" ]]       && args+=("-DQT_ANDROID_SIGN_APK=$QT_ANDROID_SIGN_APK")
[[ -n "$QT_ANDROID_SIGN_AAB" ]]       && args+=("-DQT_ANDROID_SIGN_AAB=$QT_ANDROID_SIGN_AAB")
[[ -n "$QT_ANDROID_ABIS" ]]           && args+=("-DQT_ANDROID_ABIS=$QT_ANDROID_ABIS")
[[ -n "$QT_ANDROID_BUILD_ALL_ABIS" ]] && args+=("-DQT_ANDROID_BUILD_ALL_ABIS=$QT_ANDROID_BUILD_ALL_ABIS")

if [[ -n "$FORCE" ]]; then
    run_traced rm -rf "$BUILD_PATH"
fi

run_traced cmake -S "$SOURCE_PATH" -B "$BUILD_PATH" "${args[@]}"
run_traced cmake --build "$BUILD_PATH" --config "$CMAKE_BUILD_TYPE"

[[ -n "$BUILD_AAB" ]] && run_traced cmake --build "$BUILD_PATH" --config "$CMAKE_BUILD_TYPE" -t "aab"

if [ -z "$no_installers" ]; then
    for installer in $INSTALLERS; do
        args=()
        [[ "$installer" == IFW ]] && args+=(-D "QTIFWDIR=$QIF_ROOT_PATH")

        (cd "$BUILD_PATH" && run_traced cpack -G "$installer" "${args[@]}")
    done
fi
