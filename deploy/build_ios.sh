#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

# Hold on to current directory
PROJECT_DIR=$(pwd)

BUILD_DIR=$PROJECT_DIR/build-ios
mkdir -p $BUILD_DIR

echo "Project dir: ${PROJECT_DIR}"
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist


# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
  QT_VERSION=6.6.2;
  QT_BIN_DIR=$HOME/Qt/$QT_VERSION/ios/bin
fi

echo "Using Qt in $QT_BIN_DIR"

# Checking env
$QT_BIN_DIR/qt-cmake --version
cmake --version
clang -v

# Generate XCodeProj
$QT_BIN_DIR/qt-cmake . -B $BUILD_DIR -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR -DDEPLOY=ON


cd $BUILD_DIR
xcodebuild archive \
  -project AmneziaVPN.xcodeproj \
  -scheme AmneziaVPN \
  -configuration Release \
  -archivePath ./build/AmneziaVPN.xcarchive \
  CODE_SIGNING_ALLOWED=NO \
  CODE_SIGN_IDENTITY="" \
  CODE_SIGNING_REQUIRED=NO

mkdir -p Payload

cp -R ./build/AmneziaVPN.xcarchive/Products/Applications/AmneziaVPN.app Payload/

zip -r AmneziaVPN_unsigned.ipa Payload

rm -rf Payload

echo " Build setup completed successfully."
