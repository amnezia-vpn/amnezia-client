#!/bin/bash
echo "Build script for macOS Network Extension started ..."

set -o errexit -o nounset

while getopts n flag
do
    case "${flag}" in
        n) NOTARIZE_APP=1;;
    esac
done

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
PLIST_NAME=$APP_NAME.plist

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

PREBUILT_DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/deploy-prebuilt/macos
DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos

INSTALLER_DATA_DIR=$BUILD_DIR/installer/packages/$APP_DOMAIN/data
INSTALLER_BUNDLE_DIR=$BUILD_DIR/installer/$APP_FILENAME
DMG_FILENAME=$PROJECT_DIR/${APP_NAME}.dmg

# Setup provisioning profiles for main app and NE
echo "Setting up provisioning profiles..."

# Tạo thư mục Provisioning Profiles nếu chưa tồn tại
mkdir -p ~/Library/MobileDevice/Provisioning\ Profiles

# Setup provisioning profile cho main app
echo "Setting up provisioning profile for main app (AmneziaVPN)"
cp $PROJECT_DIR/deploy/AnhTVMacOSMain.provisionprofile ~/Library/MobileDevice/Provisioning\ Profiles/
macos_main_uuid=$(grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/AnhTVMacOSMain.provisionprofile | grep -io "[-A-F0-9]\{36\}")
mv ~/Library/MobileDevice/Provisioning\ Profiles/AnhTVMacOSMain.provisionprofile ~/Library/MobileDevice/Provisioning\ Profiles/$macos_main_uuid.mobileprovision

# Setup provisioning profile cho Network Extension (NE)
echo "Setting up provisioning profile for Network Extension"
cp $PROJECT_DIR/deploy/AnhTVMacOSNE.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/macos_ne.mobileprovision
macos_ne_uuid=$(grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/macos_ne.mobileprovision | grep -io "[-A-F0-9]\{36\}")
mv ~/Library/MobileDevice/Provisioning\ Profiles/macos_ne.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/$macos_ne_uuid.mobileprovision

# Decode and install signing certificates
echo "Installing signing certificates..."
echo $MAC_TRUST_CERT_BASE64 | base64 --decode > mac_trust_cert.pem
echo $MAC_SIGNING_CERT_BASE64 | base64 --decode > mac_signing_cert.p12

# Install certificates into keychain
security create-keychain -p password build.keychain
security default-keychain -s build.keychain
security unlock-keychain -p password build.keychain
security import mac_trust_cert.pem -k build.keychain -A
security import mac_signing_cert.p12 -k build.keychain -P $MAC_SIGNING_CERT_PASSWORD -A

# Establish the keychain settings for the signing process
security set-key-partition-list -S apple-tool:,apple: -s -k password build.keychain

# Check if QIF_VERSION is properly set, otherwise set a default
if [ -z "${QIF_VERSION+x}" ]; then
  echo "QIF_VERSION is not set, using default 4.6"
  QIF_VERSION=4.6
fi

QIF_BIN_DIR="$QT_BIN_DIR/../../../Tools/QtInstallerFramework/$QIF_VERSION/bin"

# Checking environment
$QT_BIN_DIR/qt-cmake --version || { echo "Error: qt-cmake not found in $QT_BIN_DIR"; exit 1; }
cmake --version || { echo "Error: cmake not found"; exit 1; }
clang -v || { echo "Error: clang not found"; exit 1; }

# Build the Network Extension app
echo "Building Network Extension App..."
mkdir -p build-macos-ne
cd build-macos-ne

$QT_BIN_DIR/qt-cmake .. -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR -DMACOS_NE=TRUE
cmake --build . --config release --target AmneziaVPN -- -allowProvisioningUpdates # Thay đổi target phù hợp cho Network Extension

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package Network Extension
echo "Packaging Network Extension ..."

# Copy necessary data
cp -Rv $PREBUILT_DEPLOY_DATA_DIR/* $BUNDLE_DIR/Contents/macOS
$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite -qmldir=$PROJECT_DIR
cp -av $BUILD_DIR/service/server/$APP_NAME-service $BUNDLE_DIR/Contents/macOS
cp -Rv $PROJECT_DIR/deploy/data/macos/* $BUNDLE_DIR/Contents/macOS

# Signing and notarizing the Network Extension
if [ "${MAC_CERT_PW+x}" ]; then
  CERTIFICATE_P12=$DEPLOY_DIR/PrivacyTechAppleCertDeveloperId.p12
  WWDRCA=$DEPLOY_DIR/WWDRCA.cer
  KEYCHAIN=amnezia.build.macos.keychain
  TEMP_PASS=tmp_pass

  security create-keychain -p $TEMP_PASS $KEYCHAIN || true
  security default-keychain -s $KEYCHAIN
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN

  security import $WWDRCA -k $KEYCHAIN -T /usr/bin/codesign || true
  security import $CERTIFICATE_P12 -k $KEYCHAIN -P $MAC_CERT_PW -T /usr/bin/codesign || true

  echo "Signing Network Extension..."
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" $BUNDLE_DIR
  spctl -a -vvvv $BUNDLE_DIR || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing Network Extension bundle..."
    /usr/bin/ditto -c -k --keepParent $BUNDLE_DIR $PROJECT_DIR/NE_Bundle_to_notarize.zip
    xcrun notarytool submit $PROJECT_DIR/NE_Bundle_to_notarize.zip --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    rm $PROJECT_DIR/NE_Bundle_to_notarize.zip
    sleep 300
    xcrun stapler staple $BUNDLE_DIR
    spctl -a -vvvv $BUNDLE_DIR || true
  fi
fi
