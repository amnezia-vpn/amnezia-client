# syntax=docker/dockerfile:1
# escape = \


ARG QT_INSTALLATION_PREFIX="/opt/Qt"
ARG QT_INSTALLATION_RELEASE="6.11.1"
ARG QIF_INSTALLATION_RELEASE="4.11"
ARG QIF_INSTALLATION_RELEASE_CODE="411"

ARG CONAN_OUTPUT_FOLDER="/opt/conan"

ARG PROJECT_WORKDIR="/opt/amnezia/client"

# Switch from "remote" to "local" to reuse local directory instead of using "git clone"
# NOTE: when using local directory, "docker build" must be called from repository root!
ARG USE_REPO="remote"
ARG REPO_URL="https://github.com/amnezia-vpn/amnezia-client"
# Can use either branch or tag
ARG REPO_REVISION="dev"


# Stage 1. Prepare build environment (with package manager)

FROM ubuntu:22.04 AS prepare-build-env

ENV LANG C.UTF-8
ENV LANGUAGE en_US:en
ENV LC_ALL C.UTF-8

RUN --mount=type=cache,target=/var/cache/apt,sharing=locked \
    --mount=type=cache,target=/var/lib/apt,sharing=locked \
echo "Resolving external dependencies..." \
&& apt-get update \
&& apt-get install -y \
    curl build-essential git python3-pip \
    libdbus-1-dev libfreetype-dev \
    libssl-dev libssh-dev libsecret-1-dev \
    libx11-xcb-dev libxcb-xinerama0 libxcb-xkb-dev \
    libxcb-icccm4-dev libxcb-image0-dev libxcb-keysyms1-dev \
    libxcb-render-util0-dev libxcb-randr0-dev libxcb-shape0-dev \
    libxkbcommon-dev libxkbcommon-x11-dev \
    libgl1-mesa-dev libvulkan-dev


# Stage 2. Prepare Qt + Qt Installer Framework (with official online installer)

FROM prepare-build-env AS prepare-qt-env

ARG QT_INSTALLATION_PREFIX
ARG QT_INSTALLATION_RELEASE
ARG QIF_INSTALLATION_RELEASE_CODE

# NOTE: credentials are required for Qt online installer
# This can be done in two ways:
# - Provide login and password directly (useful for local builds but insecure for sure)
# - Pass qtaccount.ini to container (useful for automation, e. g. CI)
#
# Insecure option is commented out by default but you can undo it and use like this:
# `docker build --build-arg QT_CREDENTIAL_LOGIN="****@gmail.com" --build-arg QT_CREDENTIAL_PASSWORD="********"`
#ARG QT_CREDENTIAL_LOGIN=""
#ARG QT_CREDENTIAL_PASSWORD=""

RUN --mount=type=secret,id=qt_credentials,target=/root/.local/share/Qt/qtaccount.ini \
echo "Downloading Qt Online Installer..." \
&& curl -OL https://download.qt.io/official_releases/online_installers/qt-online-installer-linux-x64-online.run \
&& chmod +x ./qt-online-installer-linux-x64-online.run \
\
&& echo "Installing Qt..." \
&& ./qt-online-installer-linux-x64-online.run \
    --root ${QT_INSTALLATION_PREFIX} \
    --accept-licenses --accept-obligations --confirm-command \
    --default-answer --auto-answer OverwriteTargetDirectory=Yes \
#    --email ${QT_CREDENTIAL_LOGIN} \
#    --password ${QT_CREDENTIAL_PASSWORD} \
    install qt${QT_INSTALLATION_RELEASE}-sdk qt.tools.ifw.${QIF_INSTALLATION_RELEASE_CODE} \
\
&& rm ./qt-online-installer-linux-x64-online.run

# System-provided CMake on Ubuntu 22 is too old (3.22).
# Root CMakeLists.txt has instruction "set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES ..."
#  which is available only on CMake 3.24.
# Use instances which are already bundled with Qt
# ---
# Same for Ninja
RUN ln -s ${QT_INSTALLATION_PREFIX}/Tools/Ninja/ninja /usr/bin/ninja
RUN ln -s ${QT_INSTALLATION_PREFIX}/Tools/CMake/bin/cmake /usr/bin/cmake
RUN ln -s ${QT_INSTALLATION_PREFIX}/Tools/CMake/bin/cpack /usr/bin/cpack
RUN ln -s ${QT_INSTALLATION_PREFIX}/Tools/CMake/bin/ctest /usr/bin/ctest


# Stage 3. Prepare Conan (with "pip install")

FROM prepare-qt-env AS prepare-conan-env

RUN pip install conan

ARG REPO_URL
ARG REPO_REVISION
ARG CONAN_OUTPUT_FOLDER

# Clone remote repo, we need only Conan recipes, submodules are ignored
RUN mkdir -p /tmp/conan-build && cd /tmp/conan-build \
&& git clone --depth 1 ${REPO_URL} . \
&& git checkout ${REPO_REVISION}

# RUN --mount=type=cache,target=/root/.conan2 \
RUN \
cd /tmp/conan-build \
&& conan profile detect \
&& find recipes -name "conanfile.py" -exec conan export {} \; \
&& conan install . --build=missing --output-folder=${CONAN_OUTPUT_FOLDER}


# Stage 4. Prepare project source directory
# This is done by using either local directory or remote repo

# Stage 4 Variant A. Clone remote repo (default)

FROM prepare-conan-env AS prepare-repo-remote
ARG REPO_URL
ARG REPO_REVISION
ARG PROJECT_WORKDIR

RUN echo "Preparing AmneziaVPN Client from REMOTE repository..." \
&& mkdir -p ${PROJECT_WORKDIR} \
&& cd ${PROJECT_WORKDIR} \
&& git clone ${REPO_URL} . \
&& git checkout ${REPO_REVISION} \
&& git submodule update --init --recursive

# Stage 4 Variant B. Copy local folder (only if selected explicitly)

FROM prepare-conan-env AS prepare-repo-local
ARG PROJECT_WORKDIR

# NOTE: To use context properly, 
#  this should be launched from repository root directory
RUN --mount=type=bind,source=.,target=/tmp/local-repo \
echo "Preparing AmneziaVPN Client from LOCAL repository..." \
&& mkdir -p ${PROJECT_WORKDIR} \
&& cp -R /tmp/local-repo/. ${PROJECT_WORKDIR}/


# Stage 5. Build project from sources and create installation packages

FROM prepare-repo-${USE_REPO} AS build-package

ARG PROJECT_WORKDIR
ARG QT_INSTALLATION_PREFIX
ARG QT_INSTALLATION_RELEASE
ARG CONAN_OUTPUT_FOLDER

# Trigger multithreaded build.
# Did not set all available cores to prevent "resource exhausted"
# ("cc1 terminated" error)
ENV CMAKE_BUILD_PARALLEL_LEVEL=8

WORKDIR ${PROJECT_WORKDIR}

# Using --no-remote as we already built everything with Conan.
# --build=missing is default CONAN_INSTALL_ARGS value from conan_toolchain.cmake
RUN printf '%s\n' \
    'set(CONAN_INSTALL_ARGS "--build=missing;--no-remote" CACHE STRING "" FORCE)' \
    > ${CONAN_OUTPUT_FOLDER}/amnezia-toolchain.cmake

RUN echo "Building AmneziaVPN Client..." \
&& CMAKE_TOOLCHAIN_FILE=${CONAN_OUTPUT_FOLDER}/amnezia-toolchain.cmake \
   ./deploy/build.sh --generator Ninja --installer all -f


# Stage 6. Copy installer to final directory

# NOTE:
# Resulting image contains installer only.
# Full build environment (~20GB) is not passed to final image.
# If you need it, call "docker build --target build-package ..."
FROM scratch AS finalize

WORKDIR /dist

ARG PROJECT_WORKDIR

# Get installer we made during stage 5
COPY --from=build-package ${PROJECT_WORKDIR}/deploy/build/AmneziaVPN*.run ./
