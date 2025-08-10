#!/bin/sh
set -e

VERSION=$1

if [[ $VERSION = '' ]]; then
    echo '::error::VERSION does not set. Exiting with error...'
    exit 1
fi

mkdir -p dist

cd dist

echo $VERSION >> VERSION
curl -s https://api.github.com/repos/amnezia-vpn/amnezia-client/releases/tags/$VERSION | jq -r .body | tr -d '\r' > CHANGELOG

if [[ $(cat CHANGELOG) = null ]]; then
	echo '::error::Release does not exists. Exiting with error...'
	exit 1
fi

wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android8+_arm64-v8a.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android8+_armeabi-v7a.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android8+_x86.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android8+_x86_64.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android_7_arm64-v8a.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android_7_armeabi-v7a.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android_7_x86.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_android_7_x86_64.apk
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_linux_x64.tar.zip
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_macos.dmg
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_macos_old.dmg
wget -q https://github.com/amnezia-vpn/amnezia-client/releases/download/${VERSION}/AmneziaVPN_${VERSION}_windows_x64.exe 

cd ../

rclone sync ./dist/ r2:/updates/
