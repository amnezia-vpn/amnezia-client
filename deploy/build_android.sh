#!/bin/bash
# shellcheck disable=SC2086

set -o errexit -o nounset

usage() {
  cat <<EOT

Usage:
  build_android [options]

Build AmneziaVPN android client. By default, a signed Android App Bundle (AAB) is built.

Options:
 -d, --debug                Build debug version
 -a, --apk <abi>            Build APK for the specified ABI
                            Available ABIs: 'x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a'
 -p, --platform <platform>  The SDK platform used for building the Java code of the application
                            By default, the latest available platform is used
 -m, --move                 Move the build result to the root of the build directory
 -h, --help                 Display this help

EOT
}

BUILD_TYPE="release"
AAB=1

opts=$(getopt -l debug,apk:,platform:,move,help -o "da:p:mh" -- "$@")
eval set -- "$opts"
while true; do
  case "$1" in
    -d | --debug) BUILD_TYPE="debug"; shift;;
    -a | --apk) ABI=$2; unset AAB; shift 2;;
    -p | --platform) ANDROID_PLATFORM=$2; shift 2;;
    -m | --move) MOVE_RESULT=1; shift;;
    -h | --help) usage; exit 0;;
    --) shift; break;;
  esac
done

if [[ -v ABI && ! "$ABI" =~ ^(x86|x86_64|armeabi-v7a|arm64-v8a)$ ]]; then
  echo "The 'abi' option must be one of ['x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a'], but is '$ABI'"
  exit 1
fi

echo "Build script started..."

PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build
OUT_APP_DIR=$BUILD_DIR/client

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"

if [ -v AAB ]; then
  qt_bin_dir_suffix="x86_64"
else
  case $ABI in
    "armeabi-v7a") qt_bin_dir_suffix="armv7";;
    "arm64-v8a") qt_bin_dir_suffix="arm64_v8a";;
    *) qt_bin_dir_suffix=$ABI;;
  esac
fi
# get real path
# calls on paths containing '..' may result in a 'Permission denied'
QT_BIN_DIR=$(cd $QT_HOST_PATH/../android_$qt_bin_dir_suffix/bin && pwd)

echo "Building App..."

echo "Qt host: $QT_HOST_PATH"
echo "Using Qt in $QT_BIN_DIR"
echo "Using Android SDK in $ANDROID_SDK_ROOT"
echo "Using Android NDK in $ANDROID_NDK_ROOT"

# Run qt-cmake to configure build
qt_cmake_opts=()

if [ -v AAB ]; then
  qt_cmake_opts+=(-DQT_ANDROID_BUILD_ALL_ABIS=ON)
else
  qt_cmake_opts+=(-DQT_ANDROID_ABIS="$ABI")
fi

# QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON - Skip building apks as part of the default 'ALL' target
# We'll build apks during androiddeployqt
$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
  -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  "${qt_cmake_opts[@]}"

# Build app
cmake --build $BUILD_DIR --config $BUILD_TYPE

# Build and package APK or AAB. If this is a release, then additionally sign the result.
echo "Building APK/AAB..."

deployqt_opts=()

if [ -v AAB ]; then
  deployqt_opts+=(--aab)
fi

if [ -v ANDROID_PLATFORM ]; then
  deployqt_opts+=(--android-platform "$ANDROID_PLATFORM")
fi

if [ "$BUILD_TYPE" = "release" ]; then
  deployqt_opts+=(--release --sign)
fi

$QT_HOST_PATH/bin/androiddeployqt \
  --input $OUT_APP_DIR/android-AmneziaVPN-deployment-settings.json \
  --output $OUT_APP_DIR/android-build \
  --gradle \
  "${deployqt_opts[@]}"

if [[ -v CI || -v MOVE_RESULT ]]; then
  echo "Moving APK/AAB..."
  if [ -v AAB ]; then
    mv -u $OUT_APP_DIR/android-build/build/outputs/bundle/$BUILD_TYPE/android-build-$BUILD_TYPE.aab \
       $PROJECT_DIR/deploy/build/AmneziaVPN-$BUILD_TYPE.aab
  else
    if [ "$BUILD_TYPE" = "release" ]; then
      build_suffix="release-signed"
    else
      build_suffix=$BUILD_TYPE
    fi
    mv -u $OUT_APP_DIR/android-build/build/outputs/apk/$BUILD_TYPE/android-build-$build_suffix.apk \
       $PROJECT_DIR/deploy/build/AmneziaVPN-$ABI-$build_suffix.apk
  fi
fi
