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
PLIST_NAME=$APP_NAME.plist

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=5.15.2;
QIF_VERSION=4.1
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/ios/bin
fi

echo "Using Qt in $QT_BIN_DIR"


# Checking env
$QT_BIN_DIR/qmake -v

# Build App
echo "Building App..."
cd $PROJECT_DIR/client
$PROJECT_DIR/client/scripts/apple_compile.sh ios

# Build and run tests here

#echo "............Deploy.................."


