#!/bin/bash

function getHostTag() {
  # Copyright (C) 2010 The Android Open Source Project
  # Modified by Andy Wang (cbeuw.andy@gmail.com)
  #
  # Licensed under the Apache License, Version 2.0 (the "License");
  # you may not use this file except in compliance with the License.
  # You may obtain a copy of the License at
  #
  #      http://www.apache.org/licenses/LICENSE-2.0
  #
  # Detect host operating system and architecture
  # The 64-bit / 32-bit distinction gets tricky on Linux and Darwin because
  # uname -m returns the kernel's bit size, and it's possible to run with
  # a 64-bit kernel and a 32-bit userland.
  #
  HOST_OS=$(uname -s)
  case $HOST_OS in
  Darwin) HOST_OS=darwin ;;
  Linux) HOST_OS=linux ;;
  FreeBsd) HOST_OS=freebsd ;;
  CYGWIN* | *_NT-*) HOST_OS=windows ;;
  *)
    echo "ERROR: Unknown host operating system: $HOST_OS"
    exit 1
    ;;
  esac
  echo "HOST_OS=$HOST_OS"

  HOST_ARCH=$(uname -m)
  case $HOST_ARCH in
  i?86) HOST_ARCH=x86 ;;
  x86_64 | amd64) HOST_ARCH=x86_64 ;;
  *)
    echo "ERROR: Unknown host CPU architecture: $HOST_ARCH"
    exit 1
    ;;
  esac
  echo "HOST_ARCH=$HOST_ARCH"

  # Detect 32-bit userland on 64-bit kernels
  HOST_TAG="$HOST_OS-$HOST_ARCH"
  case $HOST_TAG in
  linux-x86_64 | darwin-x86_64)
    # we look for x86_64 or x86-64 in the output of 'file' for our shell
    # the -L flag is used to dereference symlinks, just in case.
    file -L "$SHELL" | grep -q "x86[_-]64"
    if [ $? != 0 ]; then
      HOST_ARCH=x86
      HOST_TAG=$HOST_OS-x86
      echo "HOST_ARCH=$HOST_ARCH (32-bit userland detected)"
    fi
    ;;
  esac
  # Check that we have 64-bit binaries on 64-bit system, otherwise fallback
  # on 32-bit ones. This gives us more freedom in packaging the NDK.
  if [ $HOST_ARCH = x86_64 -a ! -d $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG ]; then
    HOST_TAG=$HOST_OS-x86
    if [ $HOST_TAG = windows-x86 ]; then
      HOST_TAG=windows
    fi
    echo "HOST_TAG=$HOST_TAG (no 64-bit prebuilt binaries detected)"
  else
    echo "HOST_TAG=$HOST_TAG"
  fi
}

#Copyright (C) 2017 by Max Lv <max.c.lv@gmail.com>
#Copyright (C) 2017 by Mygod Studio <contact-shadowsocks-android@mygod.be>
#This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
#This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.

function try() {
  "$@" || exit -1
}

pushd ../..
"${ANDROID_NDK_HOME:=$(./gradlew -q printNDKPath)}"
CK_RELEASE_TAG=v"$(./gradlew -q printVersionName)"
popd

while [ ! -d "$ANDROID_NDK_HOME" ]; do
  echo "Path to ndk-bundle not found"
  exit -1
done

getHostTag
MIN_API=21
ANDROID_PREBUILT_TOOLCHAIN=$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG

ANDROID_ARM_CC=$ANDROID_PREBUILT_TOOLCHAIN/bin/armv7a-linux-androideabi${MIN_API}-clang

ANDROID_ARM64_CC=$ANDROID_PREBUILT_TOOLCHAIN/bin/aarch64-linux-android21-clang
ANDROID_ARM64_STRIP=$ANDROID_PREBUILT_TOOLCHAIN/bin/aarch64-linux-android-strip

ANDROID_X86_CC=$ANDROID_PREBUILT_TOOLCHAIN/bin/i686-linux-android${MIN_API}-clang
ANDROID_X86_STRIP=$ANDROID_PREBUILT_TOOLCHAIN/bin/i686-linux-android-strip

ANDROID_X86_64_CC=$ANDROID_PREBUILT_TOOLCHAIN/bin/x86_64-linux-android${MIN_API}-clang
ANDROID_X86_64_STRIP=$ANDROID_PREBUILT_TOOLCHAIN/bin/x86_64-linux-android-strip

DEPS=$(pwd)/client/android/cpp/.deps
SRC_DIR=$(pwd)/client/android/cpp

try mkdir -p $DEPS $SRC_DIR/cloak/armeabi-v7a $SRC_DIR/cloak/x86 $SRC_DIR/cloak/arm64-v8a $SRC_DIR/cloak/x86_64

cd $SRC_DIR/cloak
try go get ./...

cd cmd/ck-ovpn-plugin

echo "Cross compiling ckclient for arm"
try env CGO_ENABLED=1 CC="$ANDROID_ARM_CC" GOOS=android GOARCH=arm GOARM=7 go build -buildmode=c-shared -ldflags="-s -w"
try mv ck-ovpn-plugin $SRC_DIR/cloak/armeabi-v7a/libck-ovpn-plugin.so

echo "Cross compiling ckclient for arm64"
try env CGO_ENABLED=1 CC="$ANDROID_ARM64_CC" GOOS=android GOARCH=arm64 go build -buildmode=c-shared -ldflags="-s -w"
try "$ANDROID_ARM64_STRIP" ck-ovpn-plugin
try mv ck-ovpn-plugin $SRC_DIR/cloak/arm64-v8a/libck-ovpn-plugin.so

echo "Cross compiling ckclient for x86"
try env CGO_ENABLED=1 CC="$ANDROID_X86_CC" GOOS=android GOARCH=386 go build -buildmode=c-shared -ldflags="-s -w"
try "$ANDROID_X86_STRIP" ck-ovpn-plugin
try mv ck-ovpn-plugin $SRC_DIR/cloak/x86/libck-ovpn-plugin.so

echo "Cross compiling ckclient for x86_64"
try env CGO_ENABLED=1 CC="$ANDROID_X86_64_CC" GOOS=android GOARCH=amd64 go build -buildmode=c-shared -ldflags="-s -w"
try "$ANDROID_X86_64_STRIP" ck-ovpn-plugin
try mv ck-ovpn-plugin $SRC_DIR/cloak/x86_64/libck-ovpn-plugin.so

echo "Success"

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

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

# Seacrh Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.1;
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/$ANDROID_CURRENT_ARCH/bin
fi

echo "Using Qt in $QT_BIN_DIR"
echo "Using Android SDK in $ANDROID_SDK_ROOT"
echo "Using Android NDK in $ANDROID_NDK_ROOT"

# Build App
echo "Building App..."
cd $BUILD_DIR

echo "HOST Qt: $QT_HOST_PATH"

$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR \
   -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL="ON" \
   -DQT_HOST_PATH=$QT_HOST_PATH \
   -DCMAKE_BUILD_TYPE="Release"

cmake --build . --config release

echo "............APK generation.................."
cd $OUT_APP_DIR

$QT_HOST_PATH/bin/androiddeployqt \
    --output $OUT_APP_DIR/android-build \
    --gradle \
    --release \
    --input android-AmneziaVPN-deployment-settings.json \
    --android-platform android-31
   
echo "............Copy apk.................."
cp $OUT_APP_DIR/android-build/build/outputs/apk/release/android-build-release-unsigned.apk \
   $PROJECT_DIR/AmneziaVPN-release-unsigned.apk
