#!/usr/bin/env bash
#############################################################################
##
## Copyright (C) 2019 Richard Weickelt.
## Contact: https://www.qt.io/licensing/
##
## This file is part of Qbs.
##
## $QT_BEGIN_LICENSE:LGPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU Lesser General Public License Usage
## Alternatively, this file may be used under the terms of the GNU Lesser
## General Public License version 3 as published by the Free Software
## Foundation and appearing in the file LICENSE.LGPL3 included in the
## packaging of this file. Please review the following information to
## ensure the GNU Lesser General Public License version 3 requirements
## will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 2.0 or (at your option) the GNU General
## Public license version 3 or any later version approved by the KDE Free
## Qt Foundation. The licenses are as published by the Free Software
## Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-2.0.html and
## https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
set -eu

function help() {
    cat <<EOF
usage: install-qt [options] [components]

Examples
  ./install-qt.sh --version 5.13.1 qtbase
  ./install-qt.sh --version 5.14.0 --target android --toolchain any qtbase qtscript

Positional arguments
  components
        SDK components to be installed. All possible components
        can be found in the online repository at https://download.qt.io,
        for instance: qtbase, qtdeclarative, qtscript, qttools, icu, ...

        In addition to Qt packages, qtcreator is also accepted.

        The actual availability of packages may differ depending on
        target and toolchain.

Options
  -d, --directory <directory>
        Root directory where to install the components.
        Maps to C:/Qt on Windows, /opt/Qt on Linux, /usr/local/Qt on Mac
        by default.

  -f, --force
        Force download and do not attempt to re-use an existing installation.

  --host <host-os>
        The host operating system. Can be one of linux_x64, mac_x64,
        windows_x86. Auto-detected by default.

  --target <target-platform>
        The desired target platform. Can be one of desktop, android, ios.
        The default value is desktop.

  --toolchain <toolchain-type>
        The toolchain that has been used to build the binaries.
        Possible values depend on --host and --target, respectively:

            linux_x64
                android
                    any, android_armv7, android_arm64_v8a
                desktop
                    gcc_64 (default)

            mac_x64
                android
                    any, android_armv7, android_arm64_v8a
                desktop
                    clang_64 (default),
                ios
                    ios

            windows_x86
                android
                    any, android_armv7, android_arm64_v8a
                desktop
                    win64_mingw73, win64_msvc2017_64 (default)

  --version <version>
        The desired Qt version. Currently supported are all versions
        above 5.9.0.

EOF
}

TARGET_PLATFORM=desktop
COMPONENTS=
VERSION=
FORCE_DOWNLOAD=false
MD5_TOOL=md5sum

case "$OSTYPE" in
    *linux*)
        HOST_OS=linux_x64
        INSTALL_DIR=/opt/Qt
        TOOLCHAIN=gcc_64
        ;;
    *darwin*)
        HOST_OS=mac_x64
        INSTALL_DIR=/usr/local/Qt
        TOOLCHAIN=clang_64
        MD5_TOOL="md5 -r"
        ;;
    msys)
        HOST_OS=windows_x86
        INSTALL_DIR=/c/Qt
        TOOLCHAIN=win64_msvc2015_64
        ;;
    *)
        HOST_OS=
        INSTALL_DIR=
        ;;
esac

while [ $# -gt 0 ]; do
    case "$1" in
        --directory|-d)
            INSTALL_DIR="$2"
            shift
            ;;
        --force|-f)
            FORCE_DOWNLOAD=true
            ;;
        --host)
            HOST_OS="$2"
            shift
            ;;
        --target)
            TARGET_PLATFORM="$2"
            shift
            ;;
        --toolchain)
            TOOLCHAIN=$(echo $2 | tr '[A-Z]' '[a-z]')
            shift
            ;;
        --version)
            VERSION="$2"
            shift
            ;;
        --help|-h)
            help
            exit 0
            ;;
        *)
            COMPONENTS="${COMPONENTS} $1"
            ;;
    esac
    shift
done

if [ -z "${HOST_OS}" ]; then
    echo "No --host specified or auto-detection failed." >&2
    exit 1
fi

if [ -z "${INSTALL_DIR}" ]; then
    echo "No --directory specified or auto-detection failed." >&2
    exit 1
fi

if [ -z "${VERSION}" ]; then
    echo "No --version specified." >&2
    exit 1
fi

if [ -z "${COMPONENTS}" ]; then
    echo "No components specified." >&2
    exit 1
fi

case "$TARGET_PLATFORM" in
    android)
        ;;
    ios)
        ;;
    desktop)
        ;;
    *)
        echo "Error: TARGET_PLATFORM=${TARGET_PLATFORM} is not valid." >&2
        exit 1
        ;;
esac

HASH=$(echo "${OSTYPE} ${TARGET_PLATFORM} ${TOOLCHAIN} ${VERSION} ${INSTALL_DIR}" | ${MD5_TOOL} | head -c 16)
HASH_FILEPATH="${INSTALL_DIR}/${HASH}.manifest"
INSTALLATION_IS_VALID=false
if ! ${FORCE_DOWNLOAD} && [ -f "${HASH_FILEPATH}" ]; then
    INSTALLATION_IS_VALID=true
    while read filepath; do
        if [ ! -e "${filepath}" ]; then
            INSTALLATION_IS_VALID=false
            break
        fi
    done <"${HASH_FILEPATH}"
fi

if ${INSTALLATION_IS_VALID}; then
    echo "Already installed. Skipping download." >&2
    exit 0
fi

DOWNLOAD_DIR=`mktemp -d 2>/dev/null || mktemp -d -t 'install-qt'`

#
# The repository structure is a mess. Try different URL variants
#
function compute_url(){
    local COMPONENT=$1
    local CURL="curl -s -L"
    local BASE_URL="http://download.qt.io/online/qtsdkrepository/${HOST_OS}/${TARGET_PLATFORM}"
    local ANDROID_ARCH=$(echo ${TOOLCHAIN##android_})

    if [[ "${COMPONENT}" =~ "qtcreator" ]]; then

        SHORT_VERSION=${VERSION%??}
        BASE_URL="http://download.qt.io/official_releases/qtcreator"
        REMOTE_PATH="${SHORT_VERSION}/${VERSION}/installer_source/${HOST_OS}/qtcreator.7z"
        echo "${BASE_URL}/${REMOTE_PATH}"
        return 0

    else
        REMOTE_BASES=(
            # New repository format (>=6.0.0)
            "qt6_${VERSION//./}/qt.qt6.${VERSION//./}.${TOOLCHAIN}"
            "qt6_${VERSION//./}_${ANDROID_ARCH}/qt.qt6.${VERSION//./}.${TOOLCHAIN}"
            "qt6_${VERSION//./}_${ANDROID_ARCH}/qt.qt6.${VERSION//./}.${COMPONENT}.${TOOLCHAIN}"
            # New repository format (>=5.9.6)
            "qt5_${VERSION//./}/qt.qt5.${VERSION//./}.${TOOLCHAIN}"
            "qt5_${VERSION//./}/qt.qt5.${VERSION//./}.${COMPONENT}.${TOOLCHAIN}"
            # Multi-abi Android since 5.14
            "qt5_${VERSION//./}/qt.qt5.${VERSION//./}.${TARGET_PLATFORM}"
            "qt5_${VERSION//./}/qt.qt5.${VERSION//./}.${COMPONENT}.${TARGET_PLATFORM}"
            # Older repository format (<5.9.0)
            "qt5_${VERSION//./}/qt.${VERSION//./}.${TOOLCHAIN}"
            "qt5_${VERSION//./}/qt.${VERSION//./}.${COMPONENT}.${TOOLCHAIN}"
        )

        for REMOTE_BASE in ${REMOTE_BASES[*]}; do
            REMOTE_PATH="$(${CURL} ${BASE_URL}/${REMOTE_BASE}/ | grep -o -E "[[:alnum:]_.\-]*7z" | grep "${COMPONENT}" | tail -1)"
            if [ ! -z "${REMOTE_PATH}" ]; then
                echo "${BASE_URL}/${REMOTE_BASE}/${REMOTE_PATH}"
                return 0
            fi
        done
    fi

    echo "Could not determine a remote URL for ${COMPONENT} with version ${VERSION}">&2
    exit 1
}

mkdir -p ${INSTALL_DIR}
rm -f "${HASH_FILEPATH}"

for COMPONENT in ${COMPONENTS}; do

    URL="$(compute_url ${COMPONENT})"
    echo "Downloading ${COMPONENT} ${URL}..." >&2
    curl --progress-bar -L -o ${DOWNLOAD_DIR}/package.7z ${URL} >&2
    7z x -y -o${INSTALL_DIR} ${DOWNLOAD_DIR}/package.7z >/dev/null
    7z l -ba -slt -y ${DOWNLOAD_DIR}/package.7z | tr '\\' '/' | sed -n -e "s|^Path\ =\ |${INSTALL_DIR}/|p" >> "${HASH_FILEPATH}"
    rm -f ${DOWNLOAD_DIR}/package.7z

    #
    # conf file is needed for qmake
    #
    if [ "${COMPONENT}" == "qtbase" ]; then
        if [[ "${TOOLCHAIN}" =~ "win64_mingw" ]]; then
            SUBDIR="${TOOLCHAIN/win64_/}_64"
        elif [[ "${TOOLCHAIN}" =~ "win32_mingw" ]]; then
            SUBDIR="${TOOLCHAIN/win32_/}_32"
        elif [[ "${TOOLCHAIN}" =~ "win64_msvc" ]]; then
            SUBDIR="${TOOLCHAIN/win64_/}"
        elif [[ "${TOOLCHAIN}" =~ "win32_msvc" ]]; then
            SUBDIR="${TOOLCHAIN/win32_/}"
        elif [[ "${TOOLCHAIN}" =~ "any" ]] && [[ "${TARGET_PLATFORM}" == "android" ]]; then
            SUBDIR="android"
        else
            SUBDIR="${TOOLCHAIN}"
        fi

        if [ "${TARGET_PLATFORM}" == "android" ] && [ ! "${QT_VERSION}" \< "6.0.0" ]; then
            CONF_FILE="${INSTALL_DIR}/${VERSION}/${SUBDIR}/bin/target_qt.conf"
            sed -i "s|target|../$TOOLCHAIN|g" "${CONF_FILE}"
            sed -i "/HostPrefix/ s|$|gcc_64|g" "${CONF_FILE}"
            ANDROID_QMAKE_FILE="${INSTALL_DIR}/${VERSION}/${SUBDIR}/bin/qmake"
            QMAKE_FILE="${INSTALL_DIR}/${VERSION}/gcc_64/bin/qmake"
            sed -i "s|\/home\/qt\/work\/install\/bin\/qmake|$QMAKE_FILE|g" "${ANDROID_QMAKE_FILE}"
        else
            CONF_FILE="${INSTALL_DIR}/${VERSION}/${SUBDIR}/bin/qt.conf"
            echo "[Paths]" > ${CONF_FILE}
            echo "Prefix = .." >> ${CONF_FILE}
        fi

        # Adjust the license to be able to run qmake
        # sed with -i requires intermediate file on Mac OS
        PRI_FILE="${INSTALL_DIR}/${VERSION}/${SUBDIR}/mkspecs/qconfig.pri"
        sed -i.bak 's/Enterprise/OpenSource/g' "${PRI_FILE}"
        sed -i.bak 's/licheck.*//g' "${PRI_FILE}"
        rm "${PRI_FILE}.bak"

        # Print the directory so that the caller can
        # adjust the PATH variable.
        echo $(dirname "${CONF_FILE}")
    elif [[ "${COMPONENT}" =~ "qtcreator" ]]; then
        if [ "${HOST_OS}" == "mac_x64" ]; then
            echo "${INSTALL_DIR}/Qt Creator.app/Contents/MacOS"
        else
            echo "${INSTALL_DIR}/bin"
        fi
    fi

done
