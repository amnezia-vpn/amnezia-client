#!/bin/bash

XCODEBUILD="/usr/bin/xcodebuild"
WORKINGDIR=`pwd`
PATCH="/usr/bin/patch"

echo "Building OpenVPNAdapter for macOS Network Extension (MacNE)..."

# Copy the Project-MacNE.xcconfig settings to amnezia.xcconfig
cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project-MacNE.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig

# Append macOS-specific build directory configurations to amnezia.xcconfig
cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig
PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-macos
BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-macos
EOF

# Exclude UIKit, include Cocoa for macOS
# echo "OTHER_LDFLAGS = -framework Cocoa" >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig

# Fetch the current macOS SDK version dynamically
MACOSX_SDK=$(xcrun --sdk macosx --show-sdk-path | sed -E 's/.*MacOSX([0-9]+\.[0-9]+)\.sdk/\1/')

echo "Using macOS SDK version: $MACOSX_SDK"

cd 3rd/OpenVPNAdapter

# Build for macOS using the correct SDK and destination
if $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk macosx$MACOSX_SDK -destination 'generic/platform=macOS' -project OpenVPNAdapter.xcodeproj ; then
  echo "OpenVPNAdapter built successfully for macOS Network Extension (MacNE)"
else
  echo "OpenVPNAdapter macOS Network Extension (MacNE) build failed..."
fi

# Remove CodeSignature if needed for macOS
rm -rf ./build/Release-macos/OpenVPNAdapter.framework/Versions/A/_CodeSignature

cd ../../
