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

mkdir -p $DEPLOY_DIR/build-macos
BUILD_DIR=$DEPLOY_DIR/build-macos

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

echo "Import certificate"

TRUST_CERT_CER=$BUILD_DIR/trust-cert.cer
SIGNING_CERT_P12=$BUILD_DIR/signing-cert.p12

echo $MAC_TRUST_CERT_BASE64 | base64 --decode > $TRUST_CERT_CER
echo $MAC_SIGNING_CERT_BASE64 | base64 --decode > $SIGNING_CERT_P12

shasum -a 256 $TRUST_CERT_CER
shasum -a 256 $SIGNING_CERT_P12
KEYCHAIN_PASS=$MAC_SIGNING_CERT_PASSWORD

# Keychain setup
KEYCHAIN=amnezia.build.macos.keychain
TEMP_PASS=tmp_pass
KEYCHAIN_FILE=$HOME/Library/Keychains/$KEYCHAIN-db

security create-keychain -p $TEMP_PASS $KEYCHAIN || true
security default-keychain -s $KEYCHAIN
security unlock-keychain -p $TEMP_PASS $KEYCHAIN

security default-keychain
security list-keychains

# Import certificates into keychain
security import $TRUST_CERT_CER -k $KEYCHAIN -P "" -T /usr/bin/codesign || true
security import $SIGNING_CERT_P12 -k $KEYCHAIN -P $MAC_SIGNING_CERT_PASSWORD -T /usr/bin/codesign || true

# Configure keychain settings
security set-key-partition-list -S apple-tool:,apple: -k $TEMP_PASS $KEYCHAIN
security find-identity -p codesigning

# Setup provisioning profiles for main app and NE
echo "Setting up provisioning profiles..."

# Copy provisioning prifiles
mkdir -p  "$HOME/Library/MobileDevice/Provisioning Profiles/"

echo $MAC_APP_PROVISIONING_PROFILE | base64 --decode > ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision
echo $MAC_NE_PROVISIONING_PROFILE | base64 --decode > ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision

shasum -a 256 ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision
shasum -a 256 ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision

profile_uuid=`grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision | grep -io "[-A-F0-9]\{36\}"`
echo $profile_uuid
profile_ne_uuid=`grep UUID -A1 -a ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision | grep -io "[-A-F0-9]\{36\}"`
echo $profile_ne_uuid

mv ~/Library/MobileDevice/Provisioning\ Profiles/app.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/$profile_uuid.mobileprovision
mv ~/Library/MobileDevice/Provisioning\ Profiles/ne.mobileprovision ~/Library/MobileDevice/Provisioning\ Profiles/$profile_ne_uuid.mobileprovision

# setup environment
QT_MACOS_BIN=$QT_BIN_DIR
export PATH=$PATH:~/go/bin
echo "QT_BIN_DIR: $QT_BIN_DIR"


# Build the Network Extension app
echo "Building MAC Network Extension App..."
mkdir -p build-macos

$QT_MACOS_BIN/qt-cmake . -B build-macos -GXcode -DQT_HOST_PATH=$QT_MACOS_ROOT_DIR -DMACOS_NE=TRUE -DCMAKE_BUILD_TYPE=Release -DDEPLOY=ON

# Build and run tests here

echo "____________________________________"
echo "............Deploying..............."
echo "____________________________________"
echo "Deploying MAC Network Extension App..."

echo "xcode build"
xcodebuild \
"OTHER_CODE_SIGN_FLAGS=--keychain '$KEYCHAIN_FILE'" \
-configuration Release \
-scheme AmneziaVPN \
-destination "platform=macOS" \
-project $PROJECT_DIR/build-macos/AmneziaVPN.xcodeproj


# Restore keychain to default
echo "Restoring default keychain..."
security default-keychain -s "/Users/runner/Library/Keychains/login.keychain-db"

echo "Build and signing process completed successfully!"