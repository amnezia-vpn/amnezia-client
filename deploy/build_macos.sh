#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

# Hold on to current directory
PROJECT_DIR=$(pwd)
SCRIPT_DIR=$PROJECT_DIR/deploy

mkdir -p $SCRIPT_DIR/build
WORK_DIR=$SCRIPT_DIR/build

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${WORK_DIR}" 

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

RELEASE_DIR=$WORK_DIR
OUT_APP_DIR=$RELEASE_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME
DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos
INSTALLER_DATA_DIR=$RELEASE_DIR/installer/packages/$APP_DOMAIN/data

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash
TARGET_FILENAME=$PROJECT_DIR/$APP_NAME.dmg

# Seacrh Qt
if [ -z "${QT_VERSION+x}" ]; then export QT_VERSION=5.14.2; fi

QT_BIN_DIR=$HOME/Qt/$QT_VERSION/clang_64/bin
QIF_BIN_DIR=$QT_BIN_DIR/../../../Tools/QtInstallerFramework/4.0/bin

echo "Using Qt in $QT_BIN_DIR"
echo "Using QIF in $QIF_BIN_DIR"

ls -al $QT_BIN_DIR/../../..


# Checking env
$QT_BIN_DIR/qmake -v
make -v
clang -v

# Build App
echo "Building App..."
cd $WORK_DIR

$QT_BIN_DIR/qmake $PROJECT_DIR/AmneziaVPN.pro 'CONFIG+=release CONFIG+=x86_64'
make -j `sysctl -n hw.ncpu`

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package 
echo "Packaging ..."

#cd $SCRIPT_DIR

$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite
cp -av $RELEASE_DIR/service/server/$APP_NAME-service.app/Contents/macOS/$APP_NAME-service $BUNDLE_DIR/Contents/macOS
cp -Rv $PROJECT_DIR/deploy/data/macos/* $BUNDLE_DIR/Contents/macOS

if [ "${MAC_CERT_PW+x}" ]; then

CERTIFICATE_P12=$SCRIPT_DIR/PrivacyTechAppleCertDeveloperId.p12
WWDRCA=$SCRIPT_DIR/WWDRCA.cer
KEYCHAIN=build.keychain
TEMP_PASS=tmp_pass

if [ -z "$(security list-keychains | grep $KEYCHAIN)" ]; then
security create-keychain -p $TEMP_PASS $KEYCHAIN
security list-keychains
security default-keychain -s $KEYCHAIN
security unlock-keychain -p $TEMP_PASS $KEYCHAIN
security import $WWDRCA -k $KEYCHAIN -T /usr/bin/codesign
security import $CERTIFICATE_P12 -k $KEYCHAIN -P $MAC_CERT_PW -T /usr/bin/codesign
fi

security find-identity -p codesigning

codesign --deep --force --verbose -o runtime --sign "Developer ID Application: Privacy Technologies OU (X7UJ388FXK)" $BUNDLE_DIR
codesign --verify -vvvv $BUNDLE_DIR
spctl -a -vvvv $BUNDLE_DIR

fi


mkdir -p $INSTALLER_DATA_DIR
cp -av $PROJECT_DIR/deploy/installer $RELEASE_DIR
cp -av $DEPLOY_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_install.sh
cp -av $DEPLOY_DATA_DIR/post_uninstall.sh $INSTALLER_DATA_DIR/post_uninstall.sh
cp -av $DEPLOY_DATA_DIR/$PLIST_NAME $INSTALLER_DATA_DIR/$PLIST_NAME

rm -f $BUNDLE_DIR/Contents/macOS/post_install.sh $BUNDLE_DIR/Contents/macOS/post_uninstall.sh
chmod a+x $INSTALLER_DATA_DIR/post_install.sh $INSTALLER_DATA_DIR/post_uninstall.sh

cd $BUNDLE_DIR 
tar czf $INSTALLER_DATA_DIR/$APP_NAME.tar.gz ./

cd $RELEASE_DIR/installer
$QIF_BIN_DIR/binarycreator --offline-only -v -c config/macos.xml -p packages -f $APP_NAME
hdiutil create -volname $APP_NAME -srcfolder $APP_NAME.app -ov -format UDZO $TARGET_FILENAME

if [ "${MAC_CERT_PW+x}" ]; then
codesign --deep --force --verbose --sign "Developer ID Application: Privacy Technologies OU (X7UJ388FXK)" $TARGET_FILENAME
#xcrun altool --notarize-app -f $TARGET_FILENAME -t osx --primary-bundle-id $APP_DOMAIN
fi

echo "Finished, artifact is $PROJECT_DIR/$APP_NAME.dmg"
