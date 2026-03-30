#!/bin/bash
# Convenience wrapper: build the Docker image (once) then compile AmneziaVPN for Android.
#
# Run from the repository root:
#   ./deploy/build_android_docker.sh --apk arm64-v8a
#   ./deploy/build_android_docker.sh --apk all --move
#   ./deploy/build_android_docker.sh --aab --debug
#
# First run takes ~30-60 min to download Qt + Android SDK/NDK.
# Subsequent runs reuse the cached image and finish in ~10-15 min.
#
# Signed release builds – set these env vars before calling this script:
#   export ANDROID_KEYSTORE_PATH=/absolute/path/to/android.keystore
#   export ANDROID_KEYSTORE_KEY_ALIAS=myAlias
#   export ANDROID_KEYSTORE_KEY_PASS=myPass

set -o errexit -o nounset

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

IMAGE_NAME="${AMNEZIA_ANDROID_IMAGE:-amnezia-android-builder}"
DOCKERFILE="$SCRIPT_DIR/Dockerfile.android"

# ── Build the Docker image if it doesn't exist or REBUILD=1 is set ───────────
if [[ -n "${REBUILD:-}" ]] || ! docker image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    echo "Building Docker image '$IMAGE_NAME'..."
    echo "(this takes ~30-60 min on first run)"
    docker build \
        --file "$DOCKERFILE" \
        --tag "$IMAGE_NAME" \
        "$REPO_ROOT"
else
    echo "Using existing Docker image '$IMAGE_NAME'  (set REBUILD=1 to force a rebuild)"
fi

# ── Collect optional keystore env vars ───────────────────────────────────────
keystore_args=()
if [[ -n "${ANDROID_KEYSTORE_PATH:-}" ]]; then
    keystore_args+=(
        -e "ANDROID_KEYSTORE_PATH=${ANDROID_KEYSTORE_PATH}"
        -e "ANDROID_KEYSTORE_KEY_ALIAS=${ANDROID_KEYSTORE_KEY_ALIAS:-}"
        -e "ANDROID_KEYSTORE_KEY_PASS=${ANDROID_KEYSTORE_KEY_PASS:-}"
    )
fi

# ── Run the build inside Docker ───────────────────────────────────────────────
docker run --rm \
    -v "$REPO_ROOT":/src \
    "${keystore_args[@]}" \
    "$IMAGE_NAME" \
    "$@"

echo ""
echo "Build artifacts are in: $REPO_ROOT/deploy/build/"
