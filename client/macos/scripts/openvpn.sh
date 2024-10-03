XCODEBUILD="/usr/bin/xcodebuild"
WORKINGDIR=`pwd`
PATCH="/usr/bin/patch"

# Copy the Project.xcconfig settings to amnezia.xcconfig
cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig

# Append macOS-specific build directory configurations to amnezia.xcconfig
cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig
PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-macos
BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-macos
EOF

# Fetch the current macOS SDK version dynamically
MACOSX_SDK=macosx15.0
cd 3rd/OpenVPNAdapter

# Build for macOS using the correct SDK and destination
if $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk $MACOSX_SDK -destination 'generic/platform=macOS' -project OpenVPNAdapter.xcodeproj ; then
  echo "OpenVPNAdapter built successfully for macOS"
else
  echo "OpenVPNAdapter macOS build failed ..."
fi

# Remove CodeSignature if needed for macOS
rm -rf ./build/Release-macos/OpenVPNAdapter.framework/Versions/A/_CodeSignature

cd ../../
