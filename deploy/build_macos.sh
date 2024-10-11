#!/bin/bash

# If you want to just build the binary, you can pass `BUILD_ONLY=True` environment variable,
# in this case you'll get only the binary, without any deployment steps like packaging installer

# BUILD_ONLY requires:
# qt – you can install minimal QT toolchain via brew: `brew install qt`

echo "Build script started ..."

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

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.3;
QIF_VERSION=4.6
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/macos/bin
QIF_BIN_DIR=$QT_BIN_DIR/../../../Tools/QtInstallerFramework/$QIF_VERSION/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using QIF in $QIF_BIN_DIR"


# Checking env
BREW_BIN_DIR=/opt/homebrew/bin
if "$QT_BIN_DIR"/qt-cmake --version >/dev/null 2>&1; then
  QT_CMAKE=$QT_BIN_DIR/qt-cmake
  echo "qt-cmake found at $QT_BIN_DIR/qt-cmake"
else
  if $BREW_BIN_DIR/qt-cmake --version >/dev/null 2>&1; then
    QT_CMAKE=$BREW_BIN_DIR/qt-cmake
    echo "qt-cmake found at $BREW_BIN_DIR/qt-cmake"
  else
    echo "qt-cmake not found in $QT_BIN_DIR or $BREW_BIN_DIR directories."
    exit 1
  fi
fi

$QT_CMAKE --version
cmake --version
clang -v

# Build App
echo "Building App..."
cd $BUILD_DIR

$QT_CMAKE -S $PROJECT_DIR -B $BUILD_DIR
cmake --build . --config release --target all

if [ "${BUILD_ONLY:-}" = "True" ] || [ "${BUILD_ONLY:-}" = "true" ]; then
  echo Succesfull build $APP_NAME, path to binary: $BUNDLE_DIR
  exit 0
fi

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package
echo "Packaging ..."


cp -Rv $PREBUILT_DEPLOY_DATA_DIR/* $BUNDLE_DIR/Contents/macOS
$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite -qmldir=$PROJECT_DIR
cp -av $BUILD_DIR/service/server/$APP_NAME-service $BUNDLE_DIR/Contents/macOS
cp -Rv $PROJECT_DIR/deploy/data/macos/* $BUNDLE_DIR/Contents/macOS
rm -f $BUNDLE_DIR/Contents/macOS/post_install.sh $BUNDLE_DIR/Contents/macOS/post_uninstall.sh

if [ "${MAC_CERT_PW+x}" ]; then

  CERTIFICATE_P12=$DEPLOY_DIR/PrivacyTechAppleCertDeveloperId.p12
  WWDRCA=$DEPLOY_DIR/WWDRCA.cer
  KEYCHAIN=amnezia.build.macos.keychain
  TEMP_PASS=tmp_pass

  security create-keychain -p $TEMP_PASS $KEYCHAIN || true
  security default-keychain -s $KEYCHAIN
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN

  security default-keychain
  security list-keychains

  security import $WWDRCA -k $KEYCHAIN -T /usr/bin/codesign || true
  security import $CERTIFICATE_P12 -k $KEYCHAIN -P $MAC_CERT_PW -T /usr/bin/codesign || true

  security set-key-partition-list -S apple-tool:,apple: -k $TEMP_PASS $KEYCHAIN
  security find-identity -p codesigning

  echo "Signing App bundle..."
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" $BUNDLE_DIR
  /usr/bin/codesign --verify -vvvv $BUNDLE_DIR || true
  spctl -a -vvvv $BUNDLE_DIR || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing App bundle..."
    /usr/bin/ditto -c -k --keepParent $BUNDLE_DIR $PROJECT_DIR/Bundle_to_notarize.zip
    xcrun notarytool submit $PROJECT_DIR/Bundle_to_notarize.zip --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    rm $PROJECT_DIR/Bundle_to_notarize.zip
    sleep 300
    xcrun stapler staple $BUNDLE_DIR
    xcrun stapler validate $BUNDLE_DIR
    spctl -a -vvvv $BUNDLE_DIR || true
  fi
fi

echo "Packaging installer..."
mkdir -p $INSTALLER_DATA_DIR
cp -av $PROJECT_DIR/deploy/installer $BUILD_DIR
cp -av $DEPLOY_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_install.sh
cp -av $DEPLOY_DATA_DIR/post_uninstall.sh $INSTALLER_DATA_DIR/post_uninstall.sh
cp -av $DEPLOY_DATA_DIR/$PLIST_NAME $INSTALLER_DATA_DIR/$PLIST_NAME

chmod a+x $INSTALLER_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_uninstall.sh

cd $BUNDLE_DIR 
tar czf $INSTALLER_DATA_DIR/$APP_NAME.tar.gz ./

echo "Building installer..."
$QIF_BIN_DIR/binarycreator --offline-only -v -c $BUILD_DIR/installer/config/macos.xml -p $BUILD_DIR/installer/packages -f $INSTALLER_BUNDLE_DIR

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
# Allow Terminal to make changes in Privacy & Security > App Management
hdiutil create -size 256mb -volname AmneziaVPN -srcfolder $BUILD_DIR/installer/$APP_NAME.app -ov -format UDZO $DMG_FILENAME

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

echo "Finished, artifact is $DMG_FILENAME"

# restore keychain
security default-keychain -s login.keychain
