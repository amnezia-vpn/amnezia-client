
XCODEBUILD="/usr/bin/xcodebuild"
WORKINGDIR=`pwd`
PATCH="/usr/bin/patch"
 
  cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
  CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos
  BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos  
EOF


  cd 3rd/OpenVPNAdapter
  if $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project OpenVPNAdapter.xcodeproj ; then
    print Y "OpenVPNAdapter built successfully"
  else
    killProcess "OpenVPNAdapter build failed"
  fi
  cd ../../

