#!/bin/bash
# Install packages needed to build AmneziaVPN inside an Ubuntu 22.04 container.
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

apt-get update

COMMON_PKGS=(
  ca-certificates
  curl
  wget
  git
  python3
  python3-pip
  python3-venv
  python3-setuptools
  build-essential
  ninja-build
  pkg-config
  libgl1-mesa-dev
  libdbus-1-dev
  libfontconfig1-dev
  libfreetype6-dev
  libx11-xcb-dev
  libxcb-glx0-dev
  libxcb-icccm4-dev
  libxcb-image0-dev
  libxcb-keysyms1-dev
  libxcb-randr0-dev
  libxcb-render-util0-dev
  libxcb-shape0-dev
  libxcb-shm0-dev
  libxcb-sync-dev
  libxcb-xfixes0-dev
  libxcb-xinerama0-dev
  libxcb-xkb-dev
  libxcb1-dev
  libxkbcommon-dev
  libxkbcommon-x11-dev
  libsecret-1-dev
  libssl-dev
  libvulkan-dev
  mesa-common-dev
  unzip
  xz-utils
  sudo
  perl
  bison
  flex
  gperf
)

# Ubuntu 22.04 package names.
COMMON_PKGS+=(libxcb-util-dev libxcb-cursor-dev libxcb-cursor0)

apt-get install -y --no-install-recommends "${COMMON_PKGS[@]}"

# CMake >= 3.25 (distro package on 22.04 is too old for this project).
CMAKE_VERSION="${CMAKE_VERSION:-3.30.5}"
if ! cmake --version 2>/dev/null | grep -qE 'version 3\.(2[5-9]|[3-9])'; then
  curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    | tar -xz -C /usr/local --strip-components=1
fi

cmake --version
gcc --version | head -1
python3 --version
