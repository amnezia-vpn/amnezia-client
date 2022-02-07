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

compile_openvpn_adapter() {
  cd 3rd/OpenVPNAdapter
  
  $XCODEBUILD -scheme OpenVPNAdapter -configuration Debug -xcconfig Configuration/amnezia.xcconfig -sdk iphoneos -destination 'generic/platform=iOS' -project OpenVPNAdapter.xcodeproj
  
  cd ../../
}

prepare_to_build_vpn() {
  cat $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/Project.xcconfig > $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  cat << EOF >> $WORKINGDIR/3rd/OpenVPNAdapter/Configuration/amnezia.xcconfig 
  PROJECT_TEMP_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/OpenVPNAdapter.build
  CONFIGURATION_BUILD_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Debug-iphoneos
  BUILT_PRODUCTS_DIR = $WORKINGDIR/3rd/OpenVPNAdapter/build/Debug-iphoneos  
EOF
}

die() {
  if [[ "$1" ]]; then
    error "$1"
  else
    error Failed
  fi

  exit 1
}

