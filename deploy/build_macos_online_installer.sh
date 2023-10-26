#!/bin/bash
echo "___________________________________________________________________"
echo "..................repository and online installer.................."
echo "___________________________________________________________________"

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

APP_NAME=amneziavpn-online-installer
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

REPO_NAME=amneziavpn-macos-repository

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

PREBUILT_DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/deploy-prebuilt/macos
DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos

INSTALLER_DATA_DIR=$BUILD_DIR/installer/packages/$APP_DOMAIN/data
INSTALLER_BUNDLE_DIR=$BUILD_DIR/installer/$APP_FILENAME
DMG_FILENAME=$PROJECT_DIR/${APP_NAME}.dmg

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.5.1;
QIF_VERSION=4.6
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/macos/bin
QIF_BIN_DIR=$QT_BIN_DIR/../../../Tools/QtInstallerFramework/$QIF_VERSION/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using QIF in $QIF_BIN_DIR"


echo "Generating repository..."
$QIF_BIN_DIR/repogen -p $BUILD_DIR/installer/packages $BUILD_DIR/installer/$REPO_NAME

echo "Building online installer..."
$QIF_BIN_DIR/binarycreator --online-only -c $BUILD_DIR/installer/config/macos.xml -p $BUILD_DIR/installer/packages $INSTALLER_BUNDLE_DIR


if [ "${MAC_CERT_PW+x}" ]; then
  echo "Signing installer bundle..."
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" $INSTALLER_BUNDLE_DIR
  /usr/bin/codesign --verify -vvvv $INSTALLER_BUNDLE_DIR || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing installer bundle..."
    /usr/bin/ditto -c -k --keepParent $INSTALLER_BUNDLE_DIR $PROJECT_DIR/Installer_bundle_to_notarize.zip
    xcrun notarytool submit $PROJECT_DIR/Installer_bundle_to_notarize.zip --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    rm $PROJECT_DIR/Installer_bundle_to_notarize.zip
    sleep 300
    xcrun stapler staple $INSTALLER_BUNDLE_DIR
    xcrun stapler validate $INSTALLER_BUNDLE_DIR
    spctl -a -vvvv $INSTALLER_BUNDLE_DIR || true
  fi
fi

echo "Building DMG installer..."
hdiutil create -volname AmneziaVPN -srcfolder $BUILD_DIR/installer/$APP_NAME.app -ov -format UDZO $DMG_FILENAME

if [ "${MAC_CERT_PW+x}" ]; then
  echo "Signing DMG installer..."
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" $DMG_FILENAME
  /usr/bin/codesign --verify -vvvv $DMG_FILENAME || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing DMG installer..."
    xcrun notarytool submit $DMG_FILENAME --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    sleep 300
    xcrun stapler staple $DMG_FILENAME
    xcrun stapler validate $DMG_FILENAME
  fi
fi

echo "Finished to generate online instaler and repository, artifact is $DMG_FILENAME"

# restore keychain
security default-keychain -s login.keychain
