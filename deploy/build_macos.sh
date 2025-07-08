#!/bin/bash
# -----------------------------------------------------------------------------
# Usage:
#   Export the required signing credentials before running this script, e.g.:
#     export MAC_APP_CERT_PW='pw-for-DeveloperID-Application'
#     export MAC_INSTALL_CERT_PW='pw-for-DeveloperID-Installer'
#     export MAC_SIGNER_ID='Developer ID Application: Some Company Name (XXXXXXXXXX)'
#     export MAC_INSTALLER_SIGNER_ID='Developer ID Installer: Some Company Name (XXXXXXXXXX)'
#     export APPLE_DEV_EMAIL='your@email.com'
#     export APPLE_DEV_PASSWORD='<your-password>'
#     bash deploy/build_macos.sh [-n]
# -----------------------------------------------------------------------------
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
QT_VERSION=6.8.3;
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

# Create a temporary keychain and import certificates
KEYCHAIN_PATH="$PROJECT_DIR/mac_sign.keychain"
trap 'echo "Cleaning up mac_sign.keychain..."; security delete-keychain "$KEYCHAIN_PATH" 2>/dev/null || true; rm -f "$KEYCHAIN_PATH" 2>/dev/null || true' EXIT
KEYCHAIN=$(security default-keychain -d user | tr -d '"[:space:]"')

# Build a clean list of the *existing* user key-chains. The raw output of
#   security list-keychains -d user
# looks roughly like:
#     "    \"/Users/foo/Library/Keychains/login.keychain-db\"\n    \"/Library/Keychains/System.keychain\""
# Every entry is surrounded by quotes and indented with a few blanks. Feeding
# that verbatim back to `security list-keychains -s` inside a single quoted
# argument leads to one long, invalid path on some systems. We therefore strip
# the quotes and rely on the shell to split the string on whitespace so that
# each path becomes its own argument.

read -ra EXISTING_KEYCHAINS <<< "$(security list-keychains -d user | tr -d '"')"

security list-keychains -d user -s "$KEYCHAIN_PATH" "$KEYCHAIN" "${EXISTING_KEYCHAINS[@]}"
KEYCHAIN_PWD=""  # Empty password keeps things simple for CI jobs
# Create, unlock and configure the temporary key-chain so that `codesign` can
# access the imported identities without triggering interactive prompts.
security create-keychain -p "$KEYCHAIN_PWD" "$KEYCHAIN_PATH"
# Keep the key-chain unlocked for the duration of the job (6 hours is plenty).
security set-keychain-settings -lut 21600 "$KEYCHAIN_PATH"
security unlock-keychain -p "$KEYCHAIN_PWD" "$KEYCHAIN_PATH"

# Import the signing certificates only when the corresponding passwords are
# available in the environment.  This allows the script to run in environments
# where code-signing is intentionally turned off (e.g. CI jobs that just build
# the artefacts without releasing them).

if [ -n "${MAC_APP_CERT_PW-}" ]; then
  # If the certificate is provided via environment variable, decode it.
  if [ -n "${MAC_APP_CERT_CERT-}" ]; then
    echo "$MAC_APP_CERT_CERT" | base64 -d > "$DEPLOY_DIR/DeveloperIdApplicationCertificate.p12"
  fi
  security import "$DEPLOY_DIR/DeveloperIdApplicationCertificate.p12" \
          -k "$KEYCHAIN_PATH" -P "$MAC_APP_CERT_PW" -A
fi

if [ -n "${MAC_INSTALL_CERT_PW-}" ]; then
  # Same logic for the installer certificate.
  if [ -n "${MAC_INSTALLER_SIGNER_CERT-}" ]; then
    echo "$MAC_INSTALLER_SIGNER_CERT" | base64 -d > "$DEPLOY_DIR/DeveloperIdInstallerCertificate.p12"
  fi
  security import "$DEPLOY_DIR/DeveloperIdInstallerCertificate.p12" \
          -k "$KEYCHAIN_PATH" -P "$MAC_INSTALL_CERT_PW" -A
fi

# This certificate has no password.
security import "$DEPLOY_DIR/DeveloperIDG2CA.cer" -k "$KEYCHAIN_PATH" -T /usr/bin/codesign

security list-keychains -d user -s "$KEYCHAIN_PATH"

echo "____________________________________"
echo "............Deploy.................."
echo "____________________________________"

# Package
echo "Packaging ..."


cp -Rv "$PREBUILT_DEPLOY_DATA_DIR"/* "$BUNDLE_DIR/Contents/macOS"
"$QT_BIN_DIR/macdeployqt" "$OUT_APP_DIR/$APP_FILENAME" -always-overwrite -qmldir="$PROJECT_DIR"
cp -av "$BUILD_DIR/service/server/$APP_NAME-service" "$BUNDLE_DIR/Contents/macOS"
rsync -av --exclude="$PLIST_NAME" --exclude=post_install.sh --exclude=post_uninstall.sh "$DEPLOY_DATA_DIR/" "$BUNDLE_DIR/Contents/macOS/"

if [ "${MAC_APP_CERT_PW+x}" ]; then

  # Path to the p12 that contains the Developer ID *Application* certificate
  CERTIFICATE_P12=$DEPLOY_DIR/DeveloperIdApplicationCertificate.p12

  # Ensure launchd plist is bundled, but place it inside Resources so that
  # the bundle keeps a valid structure (nothing but `Contents` at the root).
  mkdir -p "$BUNDLE_DIR/Contents/Resources"
  cp "$DEPLOY_DATA_DIR/$PLIST_NAME" "$BUNDLE_DIR/Contents/Resources/$PLIST_NAME"

  # Show available signing identities (useful for debugging)
  security find-identity -p codesigning || true

  echo "Signing App bundle..."
  /usr/bin/codesign --deep --force --verbose --timestamp -o runtime --keychain "$KEYCHAIN_PATH" --sign "$MAC_SIGNER_ID" "$BUNDLE_DIR"
  /usr/bin/codesign --verify -vvvv "$BUNDLE_DIR" || true
  spctl -a -vvvv "$BUNDLE_DIR" || true

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
# launchd plist is already inside the bundle; no need to add it again after signing
/usr/bin/codesign --deep --force --verbose --timestamp -o runtime --keychain "$KEYCHAIN_PATH" --sign "$MAC_SIGNER_ID" "$PKG_ROOT/Applications/$APP_FILENAME"
/usr/bin/codesign --verify --deep --strict --verbose=4 "$PKG_ROOT/Applications/$APP_FILENAME" || true
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

# Disable bundle relocation so the app always ends up in /Applications even if
# another copy is lying around somewhere. We do this by letting pkgbuild
# analyse the contents, flipping the BundleIsRelocatable flag to false for every
# bundle it discovers and then feeding that plist back to pkgbuild.

COMPONENT_PLIST="$PKG_DIR/component.plist"
# Create the component description plist first
pkgbuild --analyze --root "$PKG_ROOT" "$COMPONENT_PLIST"

# Turn all `BundleIsRelocatable` keys to false (PlistBuddy is available on all
# macOS systems). We first convert to xml1 to ensure predictable formatting.

# Turn relocation off for every bundle entry in the plist. PlistBuddy cannot
# address keys that contain slashes without quoting, so we iterate through the
# top-level keys it prints.
plutil -convert xml1 "$COMPONENT_PLIST"
for bundle_key in $(/usr/libexec/PlistBuddy -c "Print" "$COMPONENT_PLIST" | awk '/^[ \t]*[A-Za-z0-9].*\.app/ {print $1}'); do
  /usr/libexec/PlistBuddy -c "Set :'${bundle_key}':BundleIsRelocatable false" "$COMPONENT_PLIST" || true
done

# Now build the real payload package with the edited plist so that the final
# PackageInfo contains relocatable="false".
pkgbuild --root "$PKG_ROOT" \
         --identifier "$APP_DOMAIN" \
         --version "$APP_VERSION" \
         --install-location "/" \
         --scripts "$SCRIPTS_DIR" \
         --component-plist "$COMPONENT_PLIST" \
         --sign "$MAC_INSTALLER_SIGNER_ID" \
         "$INSTALL_PKG"

# Build uninstaller component package
UNINSTALL_COMPONENT_PKG=$PKG_DIR/${APP_NAME}_uninstall_component.pkg
echo "Building uninstaller component package $UNINSTALL_COMPONENT_PKG ..."
pkgbuild --nopayload \
         --identifier "$APP_DOMAIN.uninstall" \
         --version "$APP_VERSION" \
         --scripts "$UNINSTALL_SCRIPTS_DIR" \
         --sign "$MAC_INSTALLER_SIGNER_ID" \
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
  --sign "$MAC_INSTALLER_SIGNER_ID" \
  "$UNINSTALL_PKG"

cp "$PROJECT_DIR/deploy/data/macos/distribution.xml" "$PKG_DIR/distribution.xml"

echo "Creating final installer $FINAL_PKG ..."
productbuild --distribution "$PKG_DIR/distribution.xml" \
             --package-path "$PKG_DIR" \
             --resources "$RESOURCES_DIR" \
             --sign "$MAC_INSTALLER_SIGNER_ID" \
             "$FINAL_PKG"

if [ "${MAC_INSTALL_CERT_PW+x}" ] && [ "${NOTARIZE_APP+x}" ]; then
  echo "Notarizing installer package..."
  xcrun notarytool submit "$FINAL_PKG" \
    --apple-id "$APPLE_DEV_EMAIL" \
    --team-id "$MAC_TEAM_ID" \
    --password "$APPLE_DEV_PASSWORD" \
    --wait

  echo "Stapling ticket..."
  xcrun stapler staple "$FINAL_PKG"
  xcrun stapler validate "$FINAL_PKG"
fi

if [ "${MAC_INSTALL_CERT_PW+x}" ]; then
  /usr/bin/codesign --verify -vvvv "$FINAL_PKG" || true
  spctl -a -vvvv "$FINAL_PKG" || true
fi

# Sign app bundle
/usr/bin/codesign --deep --force --verbose --timestamp -o runtime --keychain "$KEYCHAIN_PATH" --sign "$MAC_SIGNER_ID" "$BUNDLE_DIR"
spctl -a -vvvv "$BUNDLE_DIR" || true

# Restore login keychain as the only user keychain and delete the temporary keychain
KEYCHAIN="$HOME/Library/Keychains/login.keychain-db"
security list-keychains -d user -s "$KEYCHAIN"
security delete-keychain "$KEYCHAIN_PATH"

echo "Finished, artifact is $FINAL_PKG"
