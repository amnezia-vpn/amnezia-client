XCODEBUILD="/usr/bin/xcodebuild"
WORKINGDIR=`pwd`
PATCH="/usr/bin/patch"

  cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig
  cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig
  PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
  CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos
  BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos
EOF

# Lấy phiên bản SDK macOS hiện tại
MACOSX_SDK=$(xcodebuild -showsdks | grep macosx | sed -E 's/.*macosx([0-9]+\.[0-9]+).*/macosx\1/')

cd 3rd/OpenVPNAdapter
if $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk $MACOSX_SDK -destination 'generic/platform=MacOS' -project OpenVPNAdapter.xcodeproj ; then
  echo "OpenVPNAdapter built successfully"
else
  echo "OpenVPNAdapter build failed ..."
fi

rm -rf ./build/Release-iphoneos/OpenVPNAdapter.framework/Versions/A/_CodeSignature
cd ../../
