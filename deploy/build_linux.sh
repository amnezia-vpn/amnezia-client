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
PREBUILT_DEPLOY_DATA_DIR=$PROJECT_DIR/client/3rd-prebuilt/deploy-prebuilt/linux/client/bin
INSTALLER_DATA_DIR=$PROJECT_DIR/deploy/installer/packages/$APP_DOMAIN/data

PRO_FILE_PATH=$PROJECT_DIR/$APP_NAME.pro
QMAKE_STASH_FILE=$PROJECT_DIR/.qmake_stash

# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
  QT_VERSION=6.6.2
  if [ -f /opt/Qt/$QT_VERSION/gcc_64/bin/qmake ]; then
    QT_BIN_DIR=/opt/Qt/$QT_VERSION/gcc_64/bin
  elif [ -f $HOME/Qt/$QT_VERSION/gcc_64/bin/qmake ]; then
    QT_BIN_DIR=$HOME/Qt/$QT_VERSION/gcc_64/bin
  elif [ -f /usr/lib/qt6/bin/qmake ]; then
    QT_BIN_DIR=/usr/lib/qt6/bin
  elif [ -f /usr/lib/x86_64-linux-gnu/qt6/bin/qmake ]; then
    QT_BIN_DIR=/usr/lib/x86_64-linux-gnu/qt6/bin
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
cmake --build . -j$(nproc) --config release

# Build and run tests here

#echo "............Deploy.................."

cp -r $DEPLOY_DATA_DIR/* $APP_DIR
cp -r $PREBUILT_DEPLOY_DATA_DIR $APP_DIR/client

ARCH=$(uname -m)
if [ "$ARCH" = "aarch64" ] || [ "$ARCH" = "arm64" ]; then
  echo "ARM64 architecture detected - using manual deployment instead of CQtDeployer"

  # Manual deployment for ARM64
  mkdir -p $APP_DIR/client/bin
  mkdir -p $APP_DIR/service/bin

  # Copy binaries
  cp $BUILD_DIR/client/AmneziaVPN $APP_DIR/client/bin/
  cp $BUILD_DIR/service/server/AmneziaVPN-service $APP_DIR/service/bin/

  # Deploy Qt dependencies using macdeployqt-like approach with patchelf
  if ! command -v patchelf &> /dev/null; then
    echo "patchelf not found, installing..."
    apt-get update && apt-get install -y patchelf
  fi

  # Copy Qt libraries
  mkdir -p $APP_DIR/client/lib
  mkdir -p $APP_DIR/service/lib

  # Function to recursively copy library dependencies
  copy_deps() {
    local binary=$1
    local target_dir=$2

    # Get all library dependencies (excluding system libraries like libc, libm, etc)
    local deps=$(ldd "$binary" 2>/dev/null | grep "=>" | awk '{print $3}' | grep -v "^$")

    for dep in $deps; do
      if [ -f "$dep" ]; then
        local lib_name=$(basename "$dep")
        local target_path="$target_dir/$lib_name"

        # Skip if already copied
        if [ -f "$target_path" ]; then
          continue
        fi

        # Skip system libraries
        if [[ "$dep" =~ ^/lib/aarch64-linux-gnu/(libc|libm|libdl|libpthread|librt|libresolv)\.so ]]; then
          continue
        fi
        if [[ "$dep" =~ ^/lib/aarch64-linux-gnu/ld-linux-aarch64\.so ]]; then
          continue
        fi

        # Copy the library
        cp "$dep" "$target_path"

        # Recursively copy dependencies of this library
        copy_deps "$dep" "$target_dir"
      fi
    done
  }

  # Copy all dependencies for client
  echo "Copying dependencies for AmneziaVPN client..."
  copy_deps "$BUILD_DIR/client/AmneziaVPN" "$APP_DIR/client/lib"

  # Copy all dependencies for service
  echo "Copying dependencies for AmneziaVPN-service..."
  copy_deps "$BUILD_DIR/service/server/AmneziaVPN-service" "$APP_DIR/service/lib"

  # Copy Qt plugins
  mkdir -p $APP_DIR/client/plugins
  cp -r $QT_BIN_DIR/../plugins/platforms $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/imageformats $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/platformthemes $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/xcbglintegrations $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/wayland-decoration-client $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/wayland-graphics-integration-client $APP_DIR/client/plugins/ || true
  cp -r $QT_BIN_DIR/../plugins/wayland-shell-integration $APP_DIR/client/plugins/ || true

  # Copy plugin dependencies
  echo "Copying plugin dependencies..."
  for plugin in $APP_DIR/client/plugins/*/*.so; do
    if [ -f "$plugin" ]; then
      copy_deps "$plugin" "$APP_DIR/client/lib"
    fi
  done

  # Copy QML imports
  mkdir -p $APP_DIR/client/qml
  cp -r $QT_BIN_DIR/../qml/* $APP_DIR/client/qml/ || true

  # Create qt.conf files
  cat > $APP_DIR/client/bin/qt.conf << EOF
[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
Qml2Imports = qml
EOF

  cat > $APP_DIR/service/bin/qt.conf << EOF
[Paths]
Prefix = ..
Libraries = lib
Plugins = plugins
EOF

  # Set RPATH
  patchelf --set-rpath '$ORIGIN/../lib' $APP_DIR/client/bin/AmneziaVPN || true
  patchelf --set-rpath '$ORIGIN/../lib' $APP_DIR/service/bin/AmneziaVPN-service || true

else
  # x86_64 - use CQtDeployer
  if [ ! -f $CQTDEPLOYER_DIR/cqtdeployer.sh ]; then
    wget -O $TOOLS_DIR/CQtDeployer.zip https://github.com/QuasarApp/CQtDeployer/releases/download/v1.5.4.17/CQtDeployer_1.5.4.17_Linux_x86_64.zip
    unzip -o $TOOLS_DIR/CQtDeployer.zip -d $CQTDEPLOYER_DIR/
    chmod +x -R $CQTDEPLOYER_DIR
  fi

  $CQTDEPLOYER_DIR/cqtdeployer.sh -bin $BUILD_DIR/client/AmneziaVPN -qmake $QT_BIN_DIR/qmake -qmlDir $PROJECT_DIR/client/ui/qml/ -targetDir $APP_DIR/client/
  $CQTDEPLOYER_DIR/cqtdeployer.sh -bin $BUILD_DIR/service/server/AmneziaVPN-service -qmake $QT_BIN_DIR/qmake -targetDir $APP_DIR/service/
fi

rm -f $INSTALLER_DATA_DIR/data.7z

7z a $INSTALLER_DATA_DIR/data.7z $APP_DIR/*

cp -r $PROJECT_DIR/deploy/installer $BUILD_DIR

# Create installer only for x86_64 (binarycreator not available for ARM64)
if [ "$ARCH" != "aarch64" ] && [ "$ARCH" != "arm64" ]; then
  ldd $CQTDEPLOYER_DIR/bin/binarycreator
  $CQTDEPLOYER_DIR/binarycreator.sh --offline-only -v -c $BUILD_DIR/installer/config/linux.xml -p $BUILD_DIR/installer/packages -f $PROJECT_DIR/deploy/AmneziaVPN_Linux_Installer.bin
else
  echo "Skipping installer creation for ARM64 - binarycreator not available"
  echo "Deployment completed in $APP_DIR"
fi
