#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

# Hold on to current directory
PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.1;
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/$ANDROID_CURRENT_ARCH/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using Android SDK in $ANDROID_SDK_ROOT"
echo "Using Android NDK in $ANDROID_NDK_ROOT"

# Build App
echo "Building App..."
cd $BUILD_DIR

echo "HOST Qt: $QT_HOST_PATH"

$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR \
   -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL="ON" \
   -DQT_HOST_PATH=$QT_HOST_PATH \
   -DCMAKE_BUILD_TYPE="Release"

cmake --build . --config release

echo "............APK generation.................."
cd $OUT_APP_DIR

$QT_HOST_PATH/bin/androiddeployqt \
    --output $OUT_APP_DIR/android-build \
    --gradle \
    --release \
    --input android-AmneziaVPN-deployment-settings.json \
    --android-platform android-33
   
echo "............Copy apk.................."
cp $OUT_APP_DIR/android-build/build/outputs/apk/release/android-build-release-unsigned.apk \
   $PROJECT_DIR/AmneziaVPN-release-unsigned.apk
