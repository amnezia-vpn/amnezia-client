#!/bin/bash
echo "Build script started ..."

set -o errexit -o nounset

while getopts n flag
do
    case "${flag}" in
        n) NOTARIZE_APP=1;;
    esac
done

# Hold on to current directory
PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p "$DEPLOY_DIR/build"
BUILD_DIR="$DEPLOY_DIR/build"

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

# Prebuilt deployment assets are available via the symlink under deploy/data
PREBUILT_DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/deploy-prebuilt/macos
DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos


# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.3;
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/macos/bin
fi

echo "Using Qt in $QT_BIN_DIR"


# Checking env
"$QT_BIN_DIR/qt-cmake" --version
cmake --version
clang -v

# Build App
echo "Building App..."
cd "$BUILD_DIR"

"$QT_BIN_DIR/qt-cmake" -S "$PROJECT_DIR" -B "$BUILD_DIR"
cmake --build . --config release --target all

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package
echo "Packaging ..."


cp -Rv "$PREBUILT_DEPLOY_DATA_DIR"/* "$BUNDLE_DIR/Contents/macOS"
"$QT_BIN_DIR/macdeployqt" "$OUT_APP_DIR/$APP_FILENAME" -always-overwrite -qmldir="$PROJECT_DIR"
cp -av "$BUILD_DIR/service/server/$APP_NAME-service" "$BUNDLE_DIR/Contents/macOS"
rsync -av --exclude="$PLIST_NAME" --exclude=post_install.sh --exclude=post_uninstall.sh "$DEPLOY_DATA_DIR/" "$BUNDLE_DIR/Contents/macOS/"

if [ "${MAC_CERT_PW+x}" ]; then

  CERTIFICATE_P12=$DEPLOY_DIR/PrivacyTechAppleCertDeveloperId.p12
mkdir -p "$PKG_DIR" "$SCRIPTS_DIR" "$RESOURCES_DIR" "$UNINSTALL_SCRIPTS_DIR"

# Ensure launchd plist is present in the app bundle root
cp "$DEPLOY_DATA_DIR/$PLIST_NAME" "$BUNDLE_DIR/$PLIST_NAME"
pkgbuild --component "$BUNDLE_DIR" \
         --install-location "/Applications" \
  security find-identity -p codesigning

  echo "Signing App bundle..."
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" "$BUNDLE_DIR"
  /usr/bin/codesign --verify -vvvv "$BUNDLE_DIR" || true
  spctl -a -vvvv "$BUNDLE_DIR" || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing App bundle..."
    /usr/bin/ditto -c -k --keepParent "$BUNDLE_DIR" "$PROJECT_DIR/Bundle_to_notarize.zip"
    xcrun notarytool submit "$PROJECT_DIR/Bundle_to_notarize.zip" --apple-id "$APPLE_DEV_EMAIL" --team-id "$MAC_TEAM_ID" --password "$APPLE_DEV_PASSWORD"
    rm "$PROJECT_DIR/Bundle_to_notarize.zip"
    sleep 300
    xcrun stapler staple "$BUNDLE_DIR"
    xcrun stapler validate "$BUNDLE_DIR"
    spctl -a -vvvv "$BUNDLE_DIR" || true
  fi
fi

echo "Packaging installer..."
PKG_DIR=$BUILD_DIR/pkg
# Remove any stale packaging data from previous runs
rm -rf "$PKG_DIR"
PKG_ROOT=$PKG_DIR/root
SCRIPTS_DIR=$PKG_DIR/scripts
RESOURCES_DIR=$PKG_DIR/resources
INSTALL_PKG=$PKG_DIR/${APP_NAME}_install.pkg
UNINSTALL_PKG=$PKG_DIR/${APP_NAME}_uninstall.pkg
FINAL_PKG=$PKG_DIR/${APP_NAME}.pkg
UNINSTALL_SCRIPTS_DIR=$PKG_DIR/uninstall_scripts

mkdir -p "$PKG_ROOT/Applications" "$SCRIPTS_DIR" "$RESOURCES_DIR" "$UNINSTALL_SCRIPTS_DIR"

cp -R "$BUNDLE_DIR" "$PKG_ROOT/Applications"
cp "$DEPLOY_DATA_DIR/$PLIST_NAME" "$PKG_ROOT/Applications/$APP_FILENAME/$PLIST_NAME"
cp "$DEPLOY_DATA_DIR/post_install.sh" "$SCRIPTS_DIR/post_install.sh"
cp "$DEPLOY_DATA_DIR/post_uninstall.sh" "$UNINSTALL_SCRIPTS_DIR/postinstall"
mkdir -p "$RESOURCES_DIR/scripts"
cp "$DEPLOY_DATA_DIR/check_install.sh" "$RESOURCES_DIR/scripts/check_install.sh"
cp "$DEPLOY_DATA_DIR/check_uninstall.sh" "$RESOURCES_DIR/scripts/check_uninstall.sh"

cat > "$SCRIPTS_DIR/postinstall" <<'EOS'
#!/bin/bash
SCRIPT_DIR="$(dirname "$0")"
bash "$SCRIPT_DIR/post_install.sh"
exit 0
EOS

chmod +x "$SCRIPTS_DIR"/*
chmod +x "$UNINSTALL_SCRIPTS_DIR"/*
chmod +x "$RESOURCES_DIR/scripts"/*
cp "$PROJECT_DIR/LICENSE" "$RESOURCES_DIR/LICENSE"

APP_VERSION=$(grep -m1 -E 'project\(' "$PROJECT_DIR/CMakeLists.txt" | sed -E 's/.*VERSION ([0-9.]+).*/\1/')
echo "Building component package $INSTALL_PKG ..."
pkgbuild --root "$PKG_ROOT" \
         --identifier "$APP_DOMAIN" \
         --version "$APP_VERSION" \
         --install-location "/" \
         --scripts "$SCRIPTS_DIR" \
         ${MAC_INSTALLER_SIGNER_ID:+--sign "$MAC_INSTALLER_SIGNER_ID"} \
         "$INSTALL_PKG"

# Build uninstaller component package
UNINSTALL_COMPONENT_PKG=$PKG_DIR/${APP_NAME}_uninstall_component.pkg
echo "Building uninstaller component package $UNINSTALL_COMPONENT_PKG ..."
pkgbuild --nopayload \
         --identifier "$APP_DOMAIN.uninstall" \
         --version "$APP_VERSION" \
         --scripts "$UNINSTALL_SCRIPTS_DIR" \
         ${MAC_INSTALLER_SIGNER_ID:+--sign "$MAC_INSTALLER_SIGNER_ID"} \
         "$UNINSTALL_COMPONENT_PKG"

# Wrap uninstaller component in a distribution package for clearer UI
echo "Building uninstaller distribution package $UNINSTALL_PKG ..."
UNINSTALL_RESOURCES=$PKG_DIR/uninstall_resources
rm -rf "$UNINSTALL_RESOURCES"
mkdir -p "$UNINSTALL_RESOURCES"
cp "$DEPLOY_DATA_DIR/uninstall_welcome.html" "$UNINSTALL_RESOURCES"
cp "$DEPLOY_DATA_DIR/uninstall_conclusion.html" "$UNINSTALL_RESOURCES"
productbuild \
  --distribution "$DEPLOY_DATA_DIR/distribution_uninstall.xml" \
  --package-path "$PKG_DIR" \
  --resources "$UNINSTALL_RESOURCES" \
  ${MAC_INSTALLER_SIGNER_ID:+--sign "$MAC_INSTALLER_SIGNER_ID"} \
  "$UNINSTALL_PKG"

cp "$PROJECT_DIR/deploy/data/macos/distribution.xml" "$PKG_DIR/distribution.xml"

echo "Creating final installer $FINAL_PKG ..."
productbuild --distribution "$PKG_DIR/distribution.xml" \
             --package-path "$PKG_DIR" \
             --resources "$RESOURCES_DIR" \
             ${MAC_INSTALLER_SIGNER_ID:+--sign "$MAC_INSTALLER_SIGNER_ID"} \
             "$FINAL_PKG"

if [ "${MAC_CERT_PW+x}" ] && [ "${NOTARIZE_APP+x}" ]; then
  echo "Notarizing installer package..."
  xcrun notarytool submit "$FINAL_PKG" --apple-id "$APPLE_DEV_EMAIL" --team-id "$MAC_TEAM_ID" --password "$APPLE_DEV_PASSWORD"
  sleep 300
  xcrun stapler staple "$FINAL_PKG"
  xcrun stapler validate "$FINAL_PKG"
fi

if [ "${MAC_CERT_PW+x}" ]; then
  /usr/bin/codesign --verify -vvvv "$FINAL_PKG" || true
  spctl -a -vvvv "$FINAL_PKG" || true
fi

echo "Finished, artifact is $FINAL_PKG"
