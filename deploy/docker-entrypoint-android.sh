#!/bin/bash
# Entrypoint for the AmneziaVPN Android Docker builder.
# All arguments are forwarded verbatim to deploy/build_android.sh.
#
# Expected environment variables (passed via -e or the host environment):
#   ANDROID_KEYSTORE_PATH        – path to the keystore file (release builds)
#   ANDROID_KEYSTORE_KEY_ALIAS   – keystore key alias
#   ANDROID_KEYSTORE_KEY_PASS    – keystore key password
#   ANDROID_BUILD_PLATFORM       – SDK platform (default: android-36)

set -o errexit -o nounset

: "${ANDROID_BUILD_PLATFORM:=android-36}"

# Verify the source tree is mounted
if [[ ! -f /src/CMakeLists.txt ]]; then
    echo "ERROR: Source tree not found at /src."
    echo "       Mount the repository root: -v \"\$(pwd)\":/src"
    exit 1
fi

# Allow git to operate on the mounted repo and all submodules regardless of ownership.
# This is safe inside a Docker container where the build user differs from the host user.
git config --global --add safe.directory '*'

# Initialise submodules if not already done
if [[ -z "$(ls -A /src/client/3rd/shadowsocks-libev 2>/dev/null)" ]]; then
    echo "Initialising git submodules..."
    git -C /src submodule update --init --recursive
fi

export QT_HOST_PATH ANDROID_SDK_ROOT ANDROID_NDK_ROOT ANDROID_BUILD_PLATFORM

echo "=== AmneziaVPN Android Builder ==="
echo "Qt:          ${QT_VERSION}"
echo "NDK:         ${ANDROID_NDK_ROOT}"
echo "SDK platform:${ANDROID_BUILD_PLATFORM}"
echo "Arguments:   $*"
echo "=================================="

exec /src/deploy/build_android.sh \
    --build-platform "${ANDROID_BUILD_PLATFORM}" \
    "$@"
