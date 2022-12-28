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

INSTALLER_DATA_DIR=$BUILD_DIR/installer/packages/$APP_DOMAIN/data
INSTALLER_BUNDLE_DIR=$BUILD_DIR/installer/$APP_FILENAME

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash

# Seacrh Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=5.15.2;
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/android/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using Android SDK in $ANDROID_SDK_ROOT"
echo "Using Android NDK in $ANDROID_NDK_ROOT"

# Checking env
$QT_BIN_DIR/qmake -v
$ANDROID_NDK_HOME/prebuilt/linux-x86_64/bin/make -v

# Build App
echo "Building App..."
cd $BUILD_DIR

echo "HOST Qt: $QT_HOST_PATH"

$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -DQT_HOST_PATH=$QT_HOST_PATH
cmake --build . --config release -DCMAKE_BUILD_TYPE=Release

# $QT_BIN_DIR/qmake  -r -spec android-clang CONFIG+=qtquickcompiler ANDROID_ABIS="armeabi-v7a arm64-v8a x86 x86_64" $PROJECT_DIR/AmneziaVPN.pro
# echo "Executing make... may take long time"
# $ANDROID_NDK_HOME/prebuilt/linux-x86_64/bin/make -j2
# echo "Make install..."
# $ANDROID_NDK_HOME/prebuilt/linux-x86_64/bin/make install INSTALL_ROOT=android
# echo "Build OK"

# echo "............Deploy.................."
# cd $OUT_APP_DIR

# $QT_BIN_DIR/androiddeployqt \
#     --output $OUT_APP_DIR/android \
#     --gradle \
#     --release \
#     --input android-AmneziaVPN-deployment-settings.json
    
echo "............Copy apk.................."
cp $OUT_APP_DIR/android/build/outputs/apk/release/android-release-unsigned.apk \
   $PROJECT_DIR/AmneziaVPN-release-unsigned.apk
