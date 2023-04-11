#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset


# Hold on to current directory
PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build

APP_DIR=$DEPLOY_DIR/AppDir
mkdir -p $APP_DIR

TOOLS_DIR=$DEPLOY_DIR/Tools
mkdir -p $TOOLS_DIR

CQTDEPLOYER_DIR=$TOOLS_DIR/cqtdeployer
mkdir -p $CQTDEPLOYER_DIR

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package

DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/linux
INSTALLER_DATA_DIR=$PROJECT_DIR/deploy/installer/packages/$APP_DOMAIN/data

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
  QT_VERSION=5.15.2
  if [ -f /opt/Qt/$QT_VERSION/gcc_64/bin/qmake ]; then
    QT_BIN_DIR=/opt/Qt/$QT_VERSION/gcc_64/bin
  elif [ -f $HOME/Qt/$QT_VERSION/gcc_64/bin/qmake ]; then
    QT_BIN_DIR=$HOME/Qt/$QT_VERSION/gcc_64/bin
  fi
fi

echo "Using Qt in $QT_BIN_DIR"


# Checking env
$QT_BIN_DIR/qt-cmake --version
gcc -v

# Build App
echo "Building App..."
cd $BUILD_DIR

$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR
cmake --build . --config release

# Build and run tests here

#echo "............Deploy.................."

cp -r $DEPLOY_DATA_DIR/* $APP_DIR

if [ ! -f $CQTDEPLOYER_DIR/cqtdeployer.sh ]; then
  wget -O $TOOLS_DIR/CQtDeployer.zip https://github.com/QuasarApp/CQtDeployer/releases/download/v1.5.4.17/CQtDeployer_1.5.4.17_Linux_x86_64.zip
  unzip -o $TOOLS_DIR/CQtDeployer.zip -d $CQTDEPLOYER_DIR/
  chmod +x -R $CQTDEPLOYER_DIR
fi


$CQTDEPLOYER_DIR/cqtdeployer.sh -bin $BUILD_DIR/client/AmneziaVPN -qmake $QT_BIN_DIR/qmake -qmlDir $PROJECT_DIR/client/ui/qml/ -targetDir $APP_DIR/client/
$CQTDEPLOYER_DIR/cqtdeployer.sh -bin $BUILD_DIR/service/server/AmneziaVPN-service -qmake $QT_BIN_DIR/qmake -targetDir $APP_DIR/service/

rm -f $INSTALLER_DATA_DIR/data.7z

7z a $INSTALLER_DATA_DIR/data.7z $APP_DIR/*

ldd $CQTDEPLOYER_DIR/bin/binarycreator

$CQTDEPLOYER_DIR/binarycreator.sh --offline-only -v -c $PROJECT_DIR/deploy/installer/config/linux.xml -p $PROJECT_DIR/deploy/installer/packages/ -f $PROJECT_DIR/deploy/AmneziaVPN_Linux_Installer


