#!/bin/bash
set -e

# Usage:
#   deploy_s3.sh <VERSION> [--skip-release] [--from-dir <path>]
#
#   --skip-release  don't require a GitHub release (CHANGELOG empty, RELEASE_DATE = now)
#   --from-dir      use already-downloaded files instead of fetching from GitHub release

VERSION=$1
[[ -z "$VERSION" ]] && { echo "::error::VERSION is not set."; exit 1; }
shift

FROM_DIR=""
SKIP_RELEASE=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --from-dir)     FROM_DIR="$2"; shift 2 ;;
        --skip-release) SKIP_RELEASE=true; shift ;;
        *) echo "::error::Unknown argument: $1"; exit 1 ;;
    esac
done

mkdir -p dist
cd dist

echo "$VERSION" > VERSION

if [[ "$SKIP_RELEASE" == true ]]; then
    echo "" > CHANGELOG
    date -u +"%Y-%m-%dT%H:%M:%SZ" > RELEASE_DATE
else
    RELEASE_JSON=$(curl -sf "https://api.github.com/repos/amnezia-vpn/amnezia-client/releases/tags/$VERSION") || {
        echo "::error::Release not found for tag $VERSION. Use --skip-release to bypass."
        exit 1
    }
    echo "$RELEASE_JSON" | jq -r '.published_at'      > RELEASE_DATE
    echo "$RELEASE_JSON" | jq -r '.body' | tr -d '\r' > CHANGELOG
    if [[ "$(cat CHANGELOG)" == "null" ]]; then
        echo "::error::Release body is null for tag $VERSION. Use --skip-release to bypass."
        exit 1
    fi
fi

download_file() {
    local url=$1
    local filename
    filename=$(basename "$url")
    echo "Downloading $filename..."
    if ! wget -q "$url"; then
        echo "::error::Failed to download $filename from $url"
        exit 8
    fi
    echo "Downloaded $filename"
}

if [[ -n "$FROM_DIR" ]]; then
    echo "Copying files from $FROM_DIR ..."
    for ext in apk run exe msi pkg; do
        for f in "$FROM_DIR"/*."$ext"; do
            [[ -f "$f" ]] && { cp "$f" .; echo "Copied $(basename "$f")"; }
        done
    done
else
    BASE="https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}"
    download_file "${BASE}/AmneziaVPN_${VERSION}_android9+_arm64-v8a.apk"
    download_file "${BASE}/AmneziaVPN_${VERSION}_android9+_armeabi-v7a.apk"
    download_file "${BASE}/AmneziaVPN_${VERSION}_android9+_x86.apk"
    download_file "${BASE}/AmneziaVPN_${VERSION}_android9+_x86_64.apk"
    download_file "${BASE}/AmneziaVPN_${VERSION}_linux_x64.run"
    download_file "${BASE}/AmneziaVPN_${VERSION}_windows_x64.exe"
    download_file "${BASE}/AmneziaVPN_${VERSION}_windows_x64.msi"
    # macOS is not always built — skip if unavailable rather than failing
    MACOS_URL="${BASE}/AmneziaVPN_${VERSION}_macos_x64.pkg"
    if curl -sf --head "$MACOS_URL" > /dev/null 2>&1; then
        download_file "$MACOS_URL"
    else
        echo "::warning::macOS package not available for $VERSION, skipping"
    fi
fi

cd ..

# --exclude "*/**" syncs only bucket root, leaving subdirs (e.g. amneziawg/) untouched
echo "Syncing to R2..."
if ! rclone sync ./dist/ r2:/updates/ --exclude "*/**" --progress; then
    echo "::error::Failed to sync files to R2"
    exit 8
fi

echo "Deployment completed successfully!"
