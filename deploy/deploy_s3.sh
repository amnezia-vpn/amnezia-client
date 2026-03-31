#!/bin/bash
set -e

VERSION=$1
REPO_SLUG=${GITHUB_REPOSITORY:-GGmuzem/Dr.Frake-VPN}
RELEASE_API_URL="https://api.github.com/repos/${REPO_SLUG}/releases/tags/${VERSION}"
RELEASE_DOWNLOAD_BASE_URL="https://github.com/${REPO_SLUG}/releases/download/${VERSION}"

if [[ -z "$VERSION" ]]; then
    echo '::error::VERSION does not set. Exiting with error...'
    exit 1
fi

mkdir -p dist

cd dist

echo $VERSION >> VERSION
curl -s "$RELEASE_API_URL" | jq -r .body | tr -d '\r' > CHANGELOG
curl -s "$RELEASE_API_URL" | jq -r .published_at > RELEASE_DATE

if [[ $(cat CHANGELOG) = null ]]; then
	echo '::error::Release does not exists. Exiting with error...'
	exit 1
fi

download_first_available() {
    local target_name=$1
    shift

    for candidate in "$@"; do
        local url="${RELEASE_DOWNLOAD_BASE_URL}/${candidate}"
        echo "Trying ${candidate}..."
        if wget -q -O "$target_name" "$url"; then
            echo "Successfully downloaded ${candidate} -> ${target_name}"
            return 0
        fi
    done

    echo "::error::Failed to download ${target_name} from any known release asset name"
    exit 8
}

download_first_available "FBLinkVPN_${VERSION}_android9+_arm64-v8a.apk" \
    "FBLinkVPN_${VERSION}_android9+_arm64-v8a.apk" \
    "AmneziaVPN_${VERSION}_android9+_arm64-v8a.apk"

download_first_available "FBLinkVPN_${VERSION}_android9+_armeabi-v7a.apk" \
    "FBLinkVPN_${VERSION}_android9+_armeabi-v7a.apk" \
    "AmneziaVPN_${VERSION}_android9+_armeabi-v7a.apk"

download_first_available "FBLinkVPN_${VERSION}_linux_x64.tar.zip" \
    "FBLinkVPN_${VERSION}_linux_x64.tar.zip" \
    "AmneziaVPN_${VERSION}_linux_x64.tar"

download_first_available "FBLinkVPN_${VERSION}_macos.pkg" \
    "FBLinkVPN_${VERSION}_macos.pkg" \
    "AmneziaVPN_${VERSION}_macos.pkg"

download_first_available "FBLinkVPN_${VERSION}_x64.exe" \
    "FBLinkVPN_${VERSION}_x64.exe" \
    "AmneziaVPN_${VERSION}_x64.exe"

cd ../

echo "Syncing to R2..."
if ! rclone sync ./dist/ r2:/updates/; then
    echo "::error::Failed to sync files to R2"
    exit 8
fi

echo "Deployment completed successfully!"
