#!/bin/bash
set -o errexit
set +o xtrace

run_traced() {
    PS4='\033[1;34m+ \033[0m'
    set -o xtrace
    "$@"
    { set +o xtrace; } 2>/dev/null
}

: ${PROJECT_DIR:=$(pwd)}
: ${BUILD_DIR:=build}
BUILD_PATH="$PROJECT_DIR/deploy/$BUILD_DIR"

abis=()
installers=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -t|--target)        TARGET="$2";         shift 2 ;;
        -f|--force)         : ${FORCE=true};     shift   ;;
        -i|--installer)     installers+=("$2");  shift 2 ;;
        -k|--keychain)      KEYCHAIN="$2";       shift 2 ;;
        -a|--abi)           abis+=("$2");        shift 2 ;;
        -s|--sign)          : ${SIGN:=true};     shift   ;;
        --aab)              : ${BUILD_AAB=true}; shift   ;;
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

: ${HOST:="$(uname -s)"}
HOST=$(echo "$HOST" | tr '[:upper:]' '[:lower:]')
: ${TARGET:="$HOST"}
TARGET=$(echo "$TARGET" | tr '[:upper:]' '[:lower:]')

: ${INSTALLERS:="${installers[@]}"}
: ${ABIS:="${abis[@]}"}

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

: ${QT_ROOT_PATH:=$(printf '%s\n' "${qt_folders[@]}" | awk -F'/' '{print $NF, $0}' | sort -V | tail -1 | awk '{print $2}')}
: ${QIF_ROOT_PATH:=$(printf '%s\n' "${qif_folders[@]}" | awk -F'/' '{print $NF, $0}' | sort -V | tail -1 | awk '{print $2}')}

if [[ -z "$QT_ROOT_PATH" ]]; then
    echo "Qt not found in standard paths and in QT_INSTALL_DIR"
    echo "  Please install the suitable version of Qt"
    echo "  or specify it by using QT_ROOT_PATH/QT_INSTALL_DIR variables"
    exit 1
fi

case "$HOST" in
    linux)  [[ "$HOST" != "$TARGET" ]] && [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_HOST_PATH:="$QT_ROOT_PATH/gcc_64"} ;;
    darwin) [[ "$HOST" != "$TARGET" ]] && [[ -n "${QT_ROOT_PATH}" ]] && : ${QT_HOST_PATH:="$QT_ROOT_PATH/macos"} ;;
    *) echo "Unsupported host \"$HOST\""; exit 1;;
esac

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

        cmake_targets=("all")
        [[ -n "$BUILD_AAB" ]] && cmake_targets+=("aab")
        : ${CMAKE_BUILD_TARGET:="${cmake_targets[@]}"}

        if [[ -n "$SIGN" ]]; then
            QT_ANDROID_SIGN_APK=TRUE
            QT_ANDROID_SIGN_AAB=TRUE
        fi

        if [[ -z "$ABIS" ]]; then
            echo "No ABIs specified. Specify at least one using --abi option"
            exit 1
        fi

        toolchain_dir=""
        for abi in $ABIS; do
            case "$abi" in
                armeabi-v7a) : ${toolchain_dir:="android_armv7"}         ;;
                arm64-v8a)   : ${toolchain_dir:="android_arm64_v8a"}     ;;
                x86)         : ${toolchain_dir:="android_x86"}           ;;
                all|x86_64)  : ${toolchain_dir:="android_x86_64"}        ;;
                *) echo "Unsupported ABI \"${abi}\" encountered"; exit 1 ;;
            esac
        done

        if [[ "$ABIS" == "all" ]]; then
            QT_ANDROID_BUILD_ALL_ABIS=TRUE
        else
            QT_ANDROID_ABIS="${ABIS// /;}"
        fi

        : ${CMAKE_PREFIX_PATH:="$QT_ROOT_PATH/$toolchain_dir/lib/cmake/Qt6/qt.toolchain.cmake"}
        : ${CMAKE_TOOLCHAIN_FILE:="$QT_ROOT_PATH/$toolchain_dir/lib/cmake/Qt6/qt.toolchain.cmake"}
        ;;
    *) echo "Unsupported target \"$TARGET\""; exit 1;;
esac

# search for Android SDK and NDK
if [[ "$TARGET" == "android" ]]; then
    bases=()
    case "$HOST" in
        linux)  bases=(~/Android/sdk)         ;;
        darwin) bases=(~/Library/Android/sdk) ;;
    esac
    [[ -n "$ANDROID_HOME" ]] && bases=("$ANDROID_HOME" "${bases[@]}")

    ndk_dirs=()
    for sdk_dir in "${bases[@]}"; do
        [[ -d "$sdk_dir" ]] && sdk_dirs+=("$sdk_dir")

        for ndk_dir in "$sdk_dir"/ndk/${ANDROID_NDK_VERSION:-*}; do
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
[[ -n "$KEYCHAIN" ]]                  && args+=("-DBUILD_VPN_KEYCHAIN=$KEYCHAIN")
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

run_traced cmake -S "$PROJECT_DIR" -B "$BUILD_PATH" "${args[@]}" -DQT_ENABLE_VERBOSE_DEPLOYMENT=1

args=()
[[ -n "$CMAKE_BUILD_TARGET" ]] && args+=("-t" $CMAKE_BUILD_TARGET)
[[ -n "$CMAKE_BUILD_TYPE" ]]   && args+=("--config" "$CMAKE_BUILD_TYPE")

run_traced cmake --build "$BUILD_PATH" "${args[@]}"

if [ -z "$no_installers" ]; then
    for installer in $INSTALLERS; do
        (cd "$BUILD_PATH" && run_traced cpack -G "$installer" -D QTIFWDIR="$QIF_ROOT_PATH")
    done
fi
