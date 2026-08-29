#!/bin/bash
# Builds a signed Android APK with DNSTT support from a WSL2/Linux environment.
#
# libdnstt.so is no longer built here: client/cmake/android.cmake compiles the
# Go module in client/3rd/dnstt with the NDK toolchain as part of the CMake
# build, so a plain ./deploy/build.sh produces it too.
set -e

export ANDROID_HOME=${ANDROID_HOME:-/opt/android-sdk}
export ANDROID_SDK_ROOT=${ANDROID_SDK_ROOT:-$ANDROID_HOME}
export ANDROID_NDK_HOME=${ANDROID_NDK_HOME:-$ANDROID_HOME/ndk/26.3.11579264}
export ANDROID_NDK_ROOT=${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}
export QT_ROOT_PATH=${QT_ROOT_PATH:-/opt/Qt/6.10.0}
export QT_INSTALL_DIR=${QT_INSTALL_DIR:-/opt/Qt}
export JAVA_HOME=${JAVA_HOME:-/usr/lib/jvm/java-17-openjdk-amd64}
export PATH="/root/go/bin:/usr/local/go/bin:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:${PATH}"

ABI=${ABI:-arm64-v8a}
BUILD_TOOLS=${BUILD_TOOLS:-$ANDROID_HOME/build-tools/35.0.0}
KEYSTORE=${KEYSTORE:-/opt/debug.keystore}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
ANDROID_BUILD_DIR="${ROOT_DIR}/deploy/build/client/android-build"
cd "${ROOT_DIR}"

command -v go >/dev/null || { echo "go is required to build libdnstt.so" >&2; exit 1; }

echo "=== 1. Building C++ core, libdnstt.so and Qt resources ==="
bash ./deploy/build.sh -t android --abi "${ABI}"

echo "=== 2. Injecting QML plugins into libs.xml ==="
python3 "${ROOT_DIR}/deploy/patch_libs_xml.py" "${ANDROID_BUILD_DIR}"

echo "=== 3. Assembling the APK ==="
chmod +x "${ANDROID_BUILD_DIR}/gradlew"
(cd "${ANDROID_BUILD_DIR}" && ./gradlew assembleRelease)

echo "=== 4. Aligning and signing ==="
if [ ! -f "${KEYSTORE}" ]; then
    keytool -genkey -v -keystore "${KEYSTORE}" -storepass android -alias androiddebugkey \
        -keypass android -keyalg RSA -keysize 2048 -validity 10000 \
        -dname "CN=Android Debug,O=Android,C=US"
fi

UNSIGNED_APK="${ANDROID_BUILD_DIR}/build/outputs/apk/release/android-build-release-unsigned.apk"
ALIGNED_APK="$(mktemp -u /tmp/AmneziaVPN-aligned-XXXXXX.apk)"
FINAL_APK="${ROOT_DIR}/deploy/build/AmneziaVPN-dnstt.apk"

"${BUILD_TOOLS}/zipalign" -f -p 4 "${UNSIGNED_APK}" "${ALIGNED_APK}"
"${BUILD_TOOLS}/apksigner" sign --ks "${KEYSTORE}" --ks-pass pass:android --key-pass pass:android \
    --out "${FINAL_APK}" "${ALIGNED_APK}"
rm -f "${ALIGNED_APK}"

echo "APK signed -> ${FINAL_APK}"
