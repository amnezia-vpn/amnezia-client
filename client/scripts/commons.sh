#!/bin/bash

printv() {
  if [ -t 1 ]; then
    NCOLORS=$(tput colors)

    if test -n "$NCOLORS" && test "$NCOLORS" -ge 8; then
      NORMAL="$(tput sgr0)"
      RED="$(tput setaf 1)"
      GREEN="$(tput setaf 2)"
      YELLOW="$(tput setaf 3)"
    fi
  fi

  if [[ $2 = 'G' ]]; then
    # shellcheck disable=SC2086
    echo $1 -e "${GREEN}$3${NORMAL}"
  elif [[ $2 = 'Y' ]]; then
    # shellcheck disable=SC2086
    echo $1 -e "${YELLOW}$3${NORMAL}"
  elif [[ $2 = 'N' ]]; then
    # shellcheck disable=SC2086
    echo $1 -e "$3"
  else
    # shellcheck disable=SC2086
    echo $1 -e "${RED}$3${NORMAL}"
  fi
}

print() {
  printv '' "$1" "$2"
}

printn() {
  printv "-n" "$1" "$2"
}

error() {
  printv '' R "$1"
}

XCODEBUILD="/usr/bin/xcodebuild"
WORKINGDIR=`pwd`
PATCH="/usr/bin/patch"
export PATH=$GOPATH:$PATH

prepare_to_build_vpn() {
  cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
  CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos
  BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos  
EOF
}

compile_openvpn_adapter() {
  cd 3rd/OpenVPNAdapter
  if $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project OpenVPNAdapter.xcodeproj ; then
    print Y "OpenVPNAdapter built successfully"
  else
    killProcess "OpenVPNAdapter build failed"
  fi
  cd ../../
}

prepare_to_build_ss() {
  cat $WORKINGDIR/scripts/ss_ios.xcconfig > $WORKINGDIR/3rd/ShadowSocks/ss_ios.xcconfig
  cat << EOF >> $WORKINGDIR/3rd/ShadowSocks/ss_ios.xcconfig 
PROJECT_TEMP_DIR = $WORKINGDIR/3rd/ShadowSocks/build/ShadowSocks.build
CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/ShadowSocks/build/Release-iphoneos
BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/ShadowSocks/build/Release-iphoneos  
EOF
}

patch_ss() {
  cd 3rd/ShadowSocks
}

compile_ss_frameworks() {
  if $XCODEBUILD -scheme ShadowSocks -configuration Release -xcconfig ss_ios.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project ShadowSocks.xcodeproj ; then
    print Y "ShadowSocks built successfully"
  else
    killProcess "ShadowSocks build failed"
  fi
  cd ../../
}

prepare_to_build_cas() {
  cat $WORKINGDIR/scripts/cas_ios.xcconfig > $WORKINGDIR/3rd/CocoaAsyncSocket/cas_ios.xcconfig
  cat << EOF >> $WORKINGDIR/3rd/CocoaAsyncSocket/cas_ios.xcconfig
PROJECT_TEMP_DIR = $WORKINGDIR/3rd/CocoaAsyncSocket/build/CocoaAsyncSocket.build
CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/CocoaAsyncSocket/build/Release-iphoneos
BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/CocoaAsyncSocket/build/Release-iphoneos  
EOF
}

compile_cocoa_async_socket() {
  cd 3rd/CocoaAsyncSocket
  if $XCODEBUILD -scheme 'iOS Framework' -configuration Release -xcconfig cas_ios.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project CocoaAsyncSocket.xcodeproj ; then
    print Y "CocoaAsyncSocket built successfully"
  else
    killProcess "CocoaAsyncSocket build failed"
  fi
  cd ../../
}

compile_tun2socks() {
  cd 3rd/outline-go-tun2socks
  if GOOS=ios GOARCH=arm64 GOFLAGS="-tags=ios" CC=iphoneos-clang CXX=iphoneos-clang++ CGO_CFLAGS="-isysroot iphoneos -miphoneos-version-min=12.0 -fembed-bitcode -arch arm64" CGO_CXXFLAGS="-isysroot iphoneos -miphoneos-version-min=12.0 -fembed-bitcode -arch arm64" CGO_LDFLAGS="-isysroot iphoneos -miphoneos-version-min=12.0 -fembed-bitcode -arch arm64" CGO_ENABLED=1 DARWIN_SDK=iphoneos gomobile bind -a -ldflags="-w -s" -bundleid org.amnezia.tun2socks -target=ios/arm64 -tags ios -o ./build/ios/Tun2Socks.xcframework github.com/Jigsaw-Code/outline-go-tun2socks/outline/apple github.com/Jigsaw-Code/outline-go-tun2socks/outline/shadowsocks ; then
    print Y "Tun2Socks built successfully"
  else
    print "Please check that path to bin folder with gomobile is in your PATH"
    print "Usually it's in GOPATH/bin, e.g. /usr/local/go/bin"
    killProcess "Tun2Socks build failed"
  fi
  cd ../../
}

prepare_to_build_cl() {
  cat $WORKINGDIR/scripts/cl_ios.xcconfig > $WORKINGDIR/3rd/CocoaLumberjack/cl_ios.xcconfig
  cat << EOF >> $WORKINGDIR/3rd/CocoaLumberjack/cl_ios.xcconfig
PROJECT_TEMP_DIR = $WORKINGDIR/3rd/CocoaLumberjack/build/CocoaLumberjack.build
CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/CocoaLumberjack/build/Release-iphoneos
BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/CocoaLumberjack/build/Release-iphoneos  
EOF
}

compile_cocoalamberjack() {
  cd 3rd/CocoaLumberjack
  if $XCODEBUILD -scheme 'CocoaLumberjack' -configuration Release -xcconfig cl_ios.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project Lumberjack.xcodeproj ; then
    print Y "CocoaLumberjack built successfully"
  else
    killProcess "CocoaLumberjack build failed"
  fi
  cd ../../
}

killProcess() {
  if [[ "$1" ]]; then
    error "$1"
  else
    error Failed
  fi

  exit 1
}
