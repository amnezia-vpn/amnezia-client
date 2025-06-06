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

mkdir -p $DEPLOY_DIR/build
BUILD_DIR=$DEPLOY_DIR/build

echo "Project dir: ${PROJECT_DIR}" 
echo "Build dir: ${BUILD_DIR}"

APP_NAME=AmneziaVPN
APP_FILENAME=$APP_NAME.app
APP_DOMAIN=org.amneziavpn.package
PLIST_NAME=$APP_NAME.plist

OUT_APP_DIR=$BUILD_DIR/client
BUNDLE_DIR=$OUT_APP_DIR/$APP_FILENAME

PREBUILT_DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/deploy-prebuilt/macos
DEPLOY_DATA_DIR=$PROJECT_DIR/deploy/data/macos


# Search Qt
if [ -z "${QT_VERSION+x}" ]; then
QT_VERSION=6.4.3;
QT_BIN_DIR=$HOME/Qt/$QT_VERSION/macos/bin
fi

echo "Using Qt in $QT_BIN_DIR"


# Checking env
$QT_BIN_DIR/qt-cmake --version
cmake --version
clang -v

# Build App
echo "Building App..."
cd $BUILD_DIR

$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR
cmake --build . --config release --target all

# Build and run tests here

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package
echo "Packaging ..."


cp -Rv $PREBUILT_DEPLOY_DATA_DIR/* $BUNDLE_DIR/Contents/macOS
$QT_BIN_DIR/macdeployqt $OUT_APP_DIR/$APP_FILENAME -always-overwrite -qmldir=$PROJECT_DIR
cp -av $BUILD_DIR/service/server/$APP_NAME-service $BUNDLE_DIR/Contents/macOS
cp -Rv $PROJECT_DIR/deploy/data/macos/* $BUNDLE_DIR/Contents/macOS
rm -f $BUNDLE_DIR/Contents/macOS/post_install.sh $BUNDLE_DIR/Contents/macOS/post_uninstall.sh

if [ "${MAC_CERT_PW+x}" ]; then

  CERTIFICATE_P12=$DEPLOY_DIR/PrivacyTechAppleCertDeveloperId.p12
  WWDRCA=$DEPLOY_DIR/WWDRCA.cer
  KEYCHAIN=amnezia.build.macos.keychain
  TEMP_PASS=tmp_pass

  security create-keychain -p $TEMP_PASS $KEYCHAIN || true
  security default-keychain -s $KEYCHAIN
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN

  security default-keychain
  security list-keychains

  security import $WWDRCA -k $KEYCHAIN -T /usr/bin/codesign || true
  security import $CERTIFICATE_P12 -k $KEYCHAIN -P $MAC_CERT_PW -T /usr/bin/codesign || true

  security set-key-partition-list -S apple-tool:,apple: -k $TEMP_PASS $KEYCHAIN
  security find-identity -p codesigning

  echo "Signing App bundle..."
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" $BUNDLE_DIR
  /usr/bin/codesign --verify -vvvv $BUNDLE_DIR || true
  spctl -a -vvvv $BUNDLE_DIR || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing App bundle..."
    /usr/bin/ditto -c -k --keepParent $BUNDLE_DIR $PROJECT_DIR/Bundle_to_notarize.zip
    xcrun notarytool submit $PROJECT_DIR/Bundle_to_notarize.zip --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    rm $PROJECT_DIR/Bundle_to_notarize.zip
    sleep 300
    xcrun stapler staple $BUNDLE_DIR
    xcrun stapler validate $BUNDLE_DIR
    spctl -a -vvvv $BUNDLE_DIR || true
  fi
fi

echo "Packaging installer..."
PKG_DIR=$BUILD_DIR/pkg
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
         "$INSTALL_PKG"

# Build uninstaller component package
UNINSTALL_COMPONENT_PKG=$PKG_DIR/${APP_NAME}_uninstall_component.pkg
echo "Building uninstaller component package $UNINSTALL_COMPONENT_PKG ..."
pkgbuild --nopayload \
         --identifier "$APP_DOMAIN.uninstall" \
         --version "$APP_VERSION" \
         --scripts "$UNINSTALL_SCRIPTS_DIR" \
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
  "$UNINSTALL_PKG"

cp "$PROJECT_DIR/deploy/data/macos/distribution.xml" "$PKG_DIR/distribution.xml"

echo "Creating final installer $FINAL_PKG ..."
productbuild --distribution "$PKG_DIR/distribution.xml" --package-path "$PKG_DIR" --resources "$RESOURCES_DIR" "$FINAL_PKG"

if [ "${MAC_CERT_PW+x}" ]; then
  echo "Signing installer package..."
  security unlock-keychain -p $TEMP_PASS $KEYCHAIN
  /usr/bin/codesign --force --verbose --timestamp -o runtime --sign "$MAC_SIGNER_ID" "$FINAL_PKG"
  /usr/bin/codesign --verify -vvvv "$FINAL_PKG" || true

  if [ "${NOTARIZE_APP+x}" ]; then
    echo "Notarizing installer package..."
    xcrun notarytool submit "$FINAL_PKG" --apple-id $APPLE_DEV_EMAIL --team-id $MAC_TEAM_ID --password $APPLE_DEV_PASSWORD
    sleep 300
    xcrun stapler staple "$FINAL_PKG"
    xcrun stapler validate "$FINAL_PKG"
    spctl -a -vvvv "$FINAL_PKG" || true
  fi
fi

echo "Finished, artifact is $FINAL_PKG"

# restore keychain
security default-keychain -s login.keychain
