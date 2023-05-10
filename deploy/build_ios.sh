#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

# Hold on to current directory
PROJECT_DIR=$(pwd)

BUILD_DIR=$PROJECT_DIR/client/build-ios
mkdir -p $BUILD_DIR

echo "Project dir: ${PROJECT_DIR}"
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist


# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.1;
QIF_VERSION=4.1
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/ios/bin
fi

echo "Using Qt in $QT_BIN_DIR"

# Checking env
$QT_BIN_DIR/qt-cmake --version
cmake --version
clang -v

# Generate XCodeProj
$QT_BIN_DIR/qt-cmake ./client -B $BUILD_DIR -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR


# Setup keychain
if [ "${IOS_DIST_SIGNING_KEY+x}" ]; then
  echo "Import certificate"
  echo $IOS_DIST_SIGNING_KEY | base64 --decode > $BUILD_DIR/signing-cert.p12

  CERTIFICATE_P12=$BUILD_DIR/signing-cert.p12
  KEYCHAIN=amnezia.build.keychain
  TEMP_PASS=$IOS_DIST_SIGNING_KEY_PASSWORD

  security create-keychain -p $TEMP_PASS $KEYCHAIN || true
  security default-keychain -s $KEYCHAIN
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN

  security default-keychain
  security list-keychains

  security import $CERTIFICATE_P12 -k $KEYCHAIN -P $IOS_DIST_SIGNING_KEY_PASSWORD -T /usr/bin/codesign || true

  security set-key-partition-list -S "apple-tool:, apple:, codesign:" -s -k $TEMP_PASS $KEYCHAIN
  security find-identity -p codesigning
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN

# Copy provisioning prifiles
mkdir -p  "$HOME/Library/MobileDevice/Provisioning Profiles/"

echo $IOS_APP_PROVISIONING_PROFILE | base64 --decode > ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision
echo $IOS_NE_PROVISIONING_PROFILE | base64 --decode > ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision

profile_uuid=`grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision | grep -io "[-A-F0-9]\{36\}"`
profile_ne_uuid=`grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision | grep -io "[-A-F0-9]\{36\}"`

mv ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/$profile_uuid.mobileprovision
mv ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/$profile_ne_uuid.mobileprovision
fi

# Build project
xcodebuild \
"OTHER_CODE_SIGN_FLAGS=--keychain '$HOME/Library/Keychains/amnezia.build.keychain-db'" \
-configuration Release \
-scheme AmneziaVPN \
-destination "generic/platform=iOS,name=Any iOS'" \
-project $BUILD_DIR/AmneziaVPN.xcodeproj

# restore keychain
security default-keychain -s login.keychain
