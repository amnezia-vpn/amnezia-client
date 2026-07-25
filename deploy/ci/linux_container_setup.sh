#!/bin/bash
# Install packages needed to build AmneziaVPN inside an Ubuntu container (20.04 / 22.04).
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

. /etc/os-release

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

# Package names differ across Ubuntu releases.
if [[ "${VERSION_ID}" == "20.04" ]]; then
  COMMON_PKGS+=(libxcb-util0-dev gcc-10 g++-10)
else
  COMMON_PKGS+=(libxcb-util-dev libxcb-cursor-dev libxcb-cursor0)
fi

apt-get install -y --no-install-recommends "${COMMON_PKGS[@]}"

if [[ "${VERSION_ID}" == "20.04" ]]; then
  update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-10 100
  update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-10 100

  # Qt 6.x xcb platform needs libxcb-cursor, which is not in Ubuntu 20.04 repos.
  if [[ ! -f /usr/local/lib/libxcb-cursor.so ]] && [[ ! -f /usr/lib/x86_64-linux-gnu/libxcb-cursor.so.0 ]]; then
    apt-get install -y --no-install-recommends autoconf automake libtool xutils-dev libxcb-render0-dev
    tmp="$(mktemp -d)"
    # Prefer release tarball — git clone needs submodules and is fragile in CI.
    cursor_ver="0.1.4"
    curl -fsSL "https://xcb.freedesktop.org/dist/xcb-util-cursor-${cursor_ver}.tar.xz" \
      -o "${tmp}/xcb-util-cursor.tar.xz"
    tar -xJf "${tmp}/xcb-util-cursor.tar.xz" -C "${tmp}"
    (
      cd "${tmp}/xcb-util-cursor-${cursor_ver}"
      ./configure --prefix=/usr/local
      make -j"$(nproc)"
      make install
      ldconfig
    )
    rm -rf "${tmp}"
  fi
fi

# CMake >= 3.25 (distro packages on 20.04/22.04 are too old for this project).
CMAKE_VERSION="${CMAKE_VERSION:-3.30.5}"
if ! cmake --version 2>/dev/null | grep -qE 'version 3\.(2[5-9]|[3-9])'; then
  curl -fsSL "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    | tar -xz -C /usr/local --strip-components=1
fi

cmake --version
gcc --version | head -1
python3 --version
