#!/bin/bash
# shellcheck disable=SC2086

set -o errexit -o nounset

usage() {
  cat <<EOT

Usage:
  build_android [options] <artifact_types>

Build VPNNaruzhu android client.

Artifact types:
 -u, --aab                        Build Android App Bundle (AAB)
 -a, --apk (<abi_list> | all)     Build APKs for the specified ABIs or for all available ABIs
                                  Available ABIs: 'x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a'
                                  <abi_list> - list of ABIs delimited by ';'

Options:
 -d, --debug                      Build debug version
 -b, --build-platform <platform>  The SDK platform used for building the Java code of the application
                                  By default, the latest available platform is used
 -m, --move                       Move the build result to the root of the build directory
 -h, --help                       Display this help

EOT
}

BUILD_TYPE="release"

opts=$(getopt -l debug,aab,apk:,build-platform:,move,help -o "dua:b:mh" -- "$@")
eval set -- "$opts"
while true; do
  case "$1" in
    -d | --debug) BUILD_TYPE="debug"; shift;;
    -u | --aab) AAB=1; shift;;
    -a | --apk) ABIS=$2; shift 2;;
    -b | --build-platform) ANDROID_BUILD_PLATFORM=$2; shift 2;;
    -m | --move) MOVE_RESULT=1; shift;;
    -h | --help) usage; exit 0;;
    --) shift; break;;
  esac
done

# Validate ABIS parameter
if [[ -v ABIS && \
    ! "$ABIS" = "all" && \
    ! "$ABIS" =~ ^((x86|x86_64|armeabi-v7a|arm64-v8a);)*(x86|x86_64|armeabi-v7a|arm64-v8a)$ ]]; then
  echo "The 'apk' option must be a list of ['x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a']" \
       "delimited by ';' or 'all', but is '$ABIS'"
  exit 1
fi

# At least one artifact type must be specified
if [[ ! (-v AAB || -v ABIS) ]]; then
  usage; exit 0
fi

echo "Build script started..."

PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build
OUT_APP_DIR=$BUILD_DIR/client

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"

# Determine path to qt bin folder with qt-cmake
if [[ -v AAB || "$ABIS" = "all" ]]; then
  qt_bin_dir_suffix="x86_64"
else
  if [[ $ABIS = *";"* ]]; then
    oneOf=$(echo $ABIS | cut -d';' -f 1)
  else
    oneOf=$ABIS
  fi
  case $oneOf in
    "armeabi-v7a") qt_bin_dir_suffix="armv7";;
    "arm64-v8a") qt_bin_dir_suffix="arm64_v8a";;
    *) qt_bin_dir_suffix=$oneOf;;
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

if [[ -v AAB || "$ABIS" = "all" ]]; then
  qt_cmake_opts+=(-DQT_ANDROID_BUILD_ALL_ABIS=ON)
else
  qt_cmake_opts+=(-DQT_ANDROID_ABIS="$ABIS")
fi

# QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON - Skip building apks as part of the default 'ALL' target
# We'll build apks during androiddeployqt
$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
  -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  "${qt_cmake_opts[@]}"

# Build app
cmake --build $BUILD_DIR --config $BUILD_TYPE

# Build and package APK or AAB
echo "Building APK/AAB..."

deployqt_opts=()

if [ -v AAB ]; then
  deployqt_opts+=(--aab)
fi

if [ -v ANDROID_BUILD_PLATFORM ]; then
  deployqt_opts+=(--android-platform "$ANDROID_BUILD_PLATFORM")
fi

if [ "$BUILD_TYPE" = "release" ]; then
  deployqt_opts+=(--release)
fi

# for gradle to skip all tasks when it is executed by androiddeployqt
# gradle is started later explicitly
export ANDROIDDEPLOYQT_RUN=1

$QT_HOST_PATH/bin/androiddeployqt \
  --input $OUT_APP_DIR/android-VPNNaruzhu-deployment-settings.json \
  --output $OUT_APP_DIR/android-build \
  "${deployqt_opts[@]}"

# run gradle
gradle_opts=()

if [ -v AAB ]; then
  gradle_opts+=(bundle"${BUILD_TYPE^}")
fi
if [ -v ABIS ]; then
  gradle_opts+=(assemble"${BUILD_TYPE^}")
fi

$OUT_APP_DIR/android-build/gradlew \
  --project-dir $OUT_APP_DIR/android-build \
  -DexplicitRun=1 \
  "${gradle_opts[@]}"

if [[ -v CI || -v MOVE_RESULT ]]; then
  echo "Moving APK/AAB..."
  if [ -v AAB ]; then
    mv -u $OUT_APP_DIR/android-build/build/outputs/bundle/$BUILD_TYPE/VPNNaruzhu-$BUILD_TYPE.aab \
       $PROJECT_DIR/deploy/build/
  fi

  if [ -v ABIS ]; then
    if [ "$ABIS" = "all" ]; then
      ABIS="x86;x86_64;armeabi-v7a;arm64-v8a"
    fi

    IFS=';' read -r -a abi_array <<< "$ABIS"
    for ABI in "${abi_array[@]}"
    do
      mv -u $OUT_APP_DIR/android-build/build/outputs/apk/$BUILD_TYPE/VPNNaruzhu-$ABI-$BUILD_TYPE.apk \
       $PROJECT_DIR/deploy/build/
    done
  fi
fi
