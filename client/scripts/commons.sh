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

compile_openvpn_adapter() {
  cd 3rd/OpenVPNAdapter
  
  $XCODEBUILD -scheme OpenVPNAdapter -configuration Release -xcconfig Configuration/amnezia.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project OpenVPNAdapter.xcodeproj
  
  cd ../../
}


prepare_to_build_vpn() {
  cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
  CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos
  BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Release-iphoneos  
EOF
}

prepare_to_build_ss() {
  cat $WORKINGDIR/scripts/ss_ios.xcconfig > $WORKINGDIR/3rd/ShadowSocks/ss_ios.xcconfig
}

patch_ss() {
  cd 3rd/ShadowSocks
  
#  $PATCH -p1 -N --dry-run --silent < ../../scripts/ss_patch.diff 2>/dev/null
  #If the patch has not been applied then the $? which is the exit status 
  #for last command would have a success status code = 0
#  if [ $? -eq 0 ];
#  then
#    #apply the patch
#    $PATCH -p1 < ../../scripts/ss_patch.diff
#  fi
  
}

compile_ss_frameworks() {
  $XCODEBUILD -scheme ShadowSocks -configuration Release -xcconfig ss_ios.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project ShadowSocks.xcodeproj
  
  cd ../../
}

prepare_to_build_pp() {
  cat $WORKINGDIR/scripts/pp_ios.xcconfig > $WORKINGDIR/3rd/PacketProcessor/pp_ios.xcconfig
}


compile_packet_processor() {
  cd 3rd/PacketProcessor
  
  $XCODEBUILD -scheme PacketProcessor -configuration Release -xcconfig pp_ios.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project PacketProcessor.xcodeproj
  
  cd ../../
}

compile_tun2socks() {
  gomobile bind -a -ldflags="-w -s" -bundleid org.amnezia.tun2socks -target=ios/arm64 -tags ios -o ./build/ios/Tun2Socks.xcframework github.com/Jigsaw-Code/outline-go-tun2socks/outline/apple github.com/Jigsaw-Code/outline-go-tun2socks/outline/shadowsocks
}

die() {
  if [[ "$1" ]]; then
    error "$1"
  else
    error Failed
  fi

  exit 1
}
