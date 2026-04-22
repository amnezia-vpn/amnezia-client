#!/bin/bash
# shellcheck disable=SC2086

set -o errexit -o nounset

usage() {
  cat <<EOT

Usage:
  build_android [options] <artifact_types>

Build FBLink VPN android client.

Artifact types:
 -u, --aab                        Build Android App Bundle (AAB)
 -a, --apk (<abi_list> | all)     Build APKs for the specified ABIs or for all available ABIs
                                  Available ABIs: 'x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a'
                                  <abi_list> - list of ABIs delimited by ';'

Options:
 -d, --debug                      Build debug version
 -b, --build-platform <platform>  The SDK platform used for building the Java code of the application
                                  By default, the latest available platform is used
 -m, --move                       Move the build result to the root of the build directory
 -f, --fdroid                     Build for F-Droid
 -h, --help                       Display this help

EOT
}

BUILD_TYPE="release"
DEFAULT_ANDROID_ABIS="armeabi-v7a;arm64-v8a"
ALL_ANDROID_ABIS="x86;x86_64;armeabi-v7a;arm64-v8a"

opts=$(getopt -l debug,aab,apk:,build-platform:,move,fdroid,help -o "dua:b:mfh" -- "$@")
eval set -- "$opts"
while true; do
  case "$1" in
    -d | --debug) BUILD_TYPE="debug"; shift;;
    -u | --aab) AAB=1; shift;;
    -a | --apk) ABIS=$2; shift 2;;
    -b | --build-platform) ANDROID_BUILD_PLATFORM=$2; shift 2;;
    -m | --move) MOVE_RESULT=1; shift;;
    -f | --fdroid) FDROID=1; shift;;
    -h | --help) usage; exit 0;;
    --) shift; break;;
  esac
done

# Validate ABIS parameter
if [[ -v ABIS && \
    ! "$ABIS" = "all" && \
    ! "$ABIS" =~ ^((x86|x86_64|armeabi-v7a|arm64-v8a);)*(x86|x86_64|armeabi-v7a|arm64-v8a)$ ]]; then
  echo "The 'apk' option must be a list of ['x86', 'x86_64', 'armeabi-v7a', 'arm64-v8a']" \
       "delimited by ';' or 'all', but is '$ABIS'"
  exit 1
fi

# At least one artifact type must be specified
if [[ ! (-v AAB || -v ABIS) ]]; then
  usage; exit 0
fi

echo "Build script started..."

PROJECT_DIR=$(pwd)
DEPLOY_DIR=$PROJECT_DIR/deploy

mkdir -p $DEPLOY_DIR/build
ARTIFACT_DIR=$DEPLOY_DIR/build
if [[ -v AAB && -n "${ABIS:-}" ]]; then
  build_suffix="${ABIS//;/-}-aab"
elif [[ -n "${ABIS:-}" ]]; then
  build_suffix="${ABIS//;/-}"
elif [[ -v AAB ]]; then
  build_suffix="aab"
else
  build_suffix="android"
fi
BUILD_DIR=$ARTIFACT_DIR/work/$build_suffix
OUT_APP_DIR=$BUILD_DIR/client
ANDROID_DEPLOY_SETTINGS=
ANDROID_BUILD_OUT_DIR=$OUT_APP_DIR/android-build

patch_legacy_awg_package_path() {
  local target_dir=$1
  local from_pkg="org.amnezia.vpn"
  local to_pkg="com.fblink.vpn/"

  if [[ ! -d "$target_dir" ]]; then
    return 0
  fi

  local py_bin=""
  if command -v python3 >/dev/null 2>&1; then
    py_bin="python3"
  elif command -v python >/dev/null 2>&1; then
    py_bin="python"
  else
    echo "WARN: python not found; skip Android .so package-path patch"
    return 0
  fi

  "$py_bin" - "$target_dir" "$from_pkg" "$to_pkg" <<'PY'
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
old = sys.argv[2].encode("utf-8")
new = sys.argv[3].encode("utf-8")

if len(old) != len(new):
    print(f"ERROR: replacement length mismatch: {len(old)} != {len(new)}", file=sys.stderr)
    sys.exit(1)

scanned = 0
patched_files = 0
replaced_hits = 0

for so_path in root.rglob("*.so"):
    scanned += 1
    data = so_path.read_bytes()
    hit_count = data.count(old)
    if hit_count:
        so_path.write_bytes(data.replace(old, new))
        patched_files += 1
        replaced_hits += hit_count

if replaced_hits:
    print(
        f"Patched legacy package path in Android libs: "
        f"files={patched_files}, replacements={replaced_hits}, scanned={scanned}"
    )
else:
    print(f"No legacy package path found in Android libs (scanned={scanned})")
PY
}

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"

if [[ -v AAB && -z "${ABIS:-}" ]]; then
  ABIS="$DEFAULT_ANDROID_ABIS"
fi

if [[ "${ABIS:-}" = "all" ]]; then
  ABIS="$ALL_ANDROID_ABIS"
fi

ANDROID_ABIS_FOR_PACKAGING="${ABIS:-}"
CONFIGURE_ALL_ABIS=0
if [[ -n "${ABIS:-}" && "${ABIS:-}" == *";"* ]]; then
  CONFIGURE_ALL_ABIS=1
fi

# Determine path to qt bin folder with qt-cmake
if [[ $CONFIGURE_ALL_ABIS -eq 1 ]]; then
  qt_bin_dir_suffix="x86_64"
else
  if [[ $ABIS = *";"* ]]; then
    oneOf=$(echo $ABIS | cut -d';' -f 1)
  else
    oneOf=$ABIS
  fi
  case $oneOf in
    "armeabi-v7a") qt_bin_dir_suffix="armv7";;
    "arm64-v8a") qt_bin_dir_suffix="arm64_v8a";;
    *) qt_bin_dir_suffix=$oneOf;;
  esac
fi
# get real path
# calls on paths containing '..' may result in a 'Permission denied'
QT_BIN_DIR=$(cd $QT_HOST_PATH/../android_$qt_bin_dir_suffix/bin && pwd)

echo "Building App..."

echo "Qt host: $QT_HOST_PATH"
echo "Using Qt in $QT_BIN_DIR"
echo "Using Android SDK in $ANDROID_SDK_ROOT"
echo "Using Android NDK in $ANDROID_NDK_ROOT"

if [[ -f "$QT_BIN_DIR/qt-cmake" && ! -x "$QT_BIN_DIR/qt-cmake" ]]; then
  chmod +x "$QT_BIN_DIR/qt-cmake"
fi

if [[ -f "$QT_HOST_PATH/bin/androiddeployqt" && ! -x "$QT_HOST_PATH/bin/androiddeployqt" ]]; then
  chmod +x "$QT_HOST_PATH/bin/androiddeployqt"
fi

# Run qt-cmake to configure build
qt_cmake_opts=()

if [[ $CONFIGURE_ALL_ABIS -eq 1 ]]; then
  qt_cmake_opts+=(
    -DQT_ANDROID_BUILD_ALL_ABIS=ON
    -DQT_ANDROID_ABIS="$ABIS"
  )
else
  qt_cmake_opts+=(-DQT_ANDROID_ABIS="$ABIS")
fi

# QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON - Skip building apks as part of the default 'ALL' target
# We'll build apks during androiddeployqt
$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
  -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
  -DQT_USE_TARGET_ANDROID_BUILD_DIR=ON \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  "${qt_cmake_opts[@]}"

# Build app
cmake --build $BUILD_DIR --config $BUILD_TYPE

# Build and package APK or AAB
echo "Building APK/AAB..."

deployqt_opts=()

if [ -v AAB ]; then
  deployqt_opts+=(--aab)
fi

if [[ -n "${ABIS:-}" ]]; then
  deployqt_opts+=(--android-abis "$ABIS")
fi

if [ -v ANDROID_BUILD_PLATFORM ]; then
  deployqt_opts+=(--android-platform "$ANDROID_BUILD_PLATFORM")
fi

if [ "$BUILD_TYPE" = "release" ]; then
  deployqt_opts+=(--release)
fi

# Resolve the deployment settings file dynamically so the script survives
# target/binary renames and does not rely on stale generated names.
if [[ -f "$OUT_APP_DIR/android-FBLink-deployment-settings.json" ]]; then
  ANDROID_DEPLOY_SETTINGS="$OUT_APP_DIR/android-FBLink-deployment-settings.json"
else
  deployment_settings=("$OUT_APP_DIR"/android-*-deployment-settings.json)
  if [[ -f "${deployment_settings[0]}" ]]; then
    ANDROID_DEPLOY_SETTINGS="${deployment_settings[0]}"
  fi
fi

if [[ -z "${ANDROID_DEPLOY_SETTINGS:-}" || ! -f "$ANDROID_DEPLOY_SETTINGS" ]]; then
  echo "ERROR: Android deployment settings were not generated in $OUT_APP_DIR"
  exit 1
fi

# Remove stale androiddeployqt output so manifest/package/activity changes are
# copied from source instead of reusing an old android-build-* tree.
rm -rf "$ANDROID_BUILD_OUT_DIR"

# Qt 6.10 CI builds sometimes produce the application .so in the CMake build
# tree but fail to copy it into android-build/libs/<abi> before packaging.
# Pre-stage the main binary so androiddeployqt can proceed consistently.
if [[ -n "${ANDROID_ABIS_FOR_PACKAGING:-}" ]]; then
  IFS=';' read -r -a abi_array <<< "$ANDROID_ABIS_FOR_PACKAGING"
  for ABI in "${abi_array[@]}"
  do
    DEST_DIR="$ANDROID_BUILD_OUT_DIR/libs/$ABI"
    DEST_FILE="$DEST_DIR/libFBLink_$ABI.so"
    ABI_UNDERSCORES="${ABI//-/_}"
    mkdir -p "$DEST_DIR"

    SRC_FILE=""
    for CANDIDATE in \
      "$OUT_APP_DIR/libFBLink_$ABI.so" \
      "$OUT_APP_DIR/libFBLink_$ABI_UNDERSCORES.so" \
      "$OUT_APP_DIR/libFBLink.so" \
      "$BUILD_DIR/client/libFBLink_$ABI.so" \
      "$BUILD_DIR/client/libFBLink_$ABI_UNDERSCORES.so" \
      "$BUILD_DIR/client/libFBLink.so" \
      "$BUILD_DIR/libFBLink_$ABI.so" \
      "$BUILD_DIR/libFBLink_$ABI_UNDERSCORES.so" \
      "$BUILD_DIR/libFBLink.so"
    do
      if [[ -f "$CANDIDATE" ]]; then
        SRC_FILE="$CANDIDATE"
        break
      fi
    done

    if [[ -z "$SRC_FILE" ]]; then
      SRC_FILE=$(find "$BUILD_DIR" -type f \( \
        -name "libFBLink_$ABI.so" -o \
        -name "libFBLink_$ABI_UNDERSCORES.so" -o \
        -name "libFBLink.so" \
      \) 2>/dev/null | head -1 || true)
    fi

    if [[ -n "$SRC_FILE" && -f "$SRC_FILE" ]]; then
      cp "$SRC_FILE" "$DEST_FILE"
      echo "Pre-staged application binary for $ABI: $SRC_FILE -> $DEST_FILE"
    else
      echo "WARN: Could not pre-stage libFBLink_$ABI.so before androiddeployqt"
    fi
  done
fi

# for gradle to skip all tasks when it is executed by androiddeployqt
# gradle is started later explicitly
export ANDROIDDEPLOYQT_RUN=1

$QT_HOST_PATH/bin/androiddeployqt \
  --input "$ANDROID_DEPLOY_SETTINGS" \
  --output "$ANDROID_BUILD_OUT_DIR" \
  "${deployqt_opts[@]}"

# Some upstream AWG binaries still contain a hardcoded legacy package id
# (org.amnezia.vpn), which breaks runtime startup in com.fblink.vpn builds.
# Patch copied binaries in android-build before Gradle packaging.
patch_legacy_awg_package_path "$ANDROID_BUILD_OUT_DIR"

# run gradle
gradle_opts=()

if [ -v FDROID ]; then
  BUILD_TYPE="fdroid"
fi

if [ -v AAB ]; then
  gradle_opts+=(bundle"${BUILD_TYPE^}")
fi
if [ -v ABIS ]; then
  gradle_opts+=(assemble"${BUILD_TYPE^}")
fi

"$ANDROID_BUILD_OUT_DIR/gradlew" \
  --project-dir "$ANDROID_BUILD_OUT_DIR" \
  -DexplicitRun=1 \
  "${gradle_opts[@]}"

if [[ -v CI || -v MOVE_RESULT ]]; then
  echo "Moving APK/AAB..."
  if [ -v AAB ]; then
    AAB_FILE=$(find "$ANDROID_BUILD_OUT_DIR" -type f -name "*.aab" 2>/dev/null | head -1)
    if [ -z "$AAB_FILE" ]; then
      echo "ERROR: AAB not found under $ANDROID_BUILD_OUT_DIR"
      exit 1
    fi
    mv -u "$AAB_FILE" $ARTIFACT_DIR/FBLink-$BUILD_TYPE.aab
  fi

  if [ -v ABIS ]; then
    suffix=$BUILD_TYPE
    if [ -v FDROID ]; then
      suffix+="-unsigned"
    fi

    APK_OUT_DIR=$ANDROID_BUILD_OUT_DIR/build/outputs/apk/$BUILD_TYPE

    # Qt 6.7+ with QT_ANDROID_BUILD_ALL_ABIS=ON produces a single universal APK
    # instead of separate per-ABI files.
    UNIVERSAL_APK_SIGNED=$(find "$APK_OUT_DIR" -maxdepth 1 -type f -name "*$suffix*.apk" ! -name "*unsigned*.apk" 2>/dev/null | head -1)
    UNIVERSAL_APK_UNSIGNED=$(find "$APK_OUT_DIR" -maxdepth 1 -type f -name "*$suffix*unsigned*.apk" 2>/dev/null | head -1)
    # Prefer a universal output when available.
    if [ -f "$UNIVERSAL_APK_SIGNED" ]; then
      mv -u "$UNIVERSAL_APK_SIGNED" $ARTIFACT_DIR/FBLink-$suffix.apk
    elif [ -f "$UNIVERSAL_APK_UNSIGNED" ] && [ -v FDROID ]; then
      mv -u "$UNIVERSAL_APK_UNSIGNED" $ARTIFACT_DIR/FBLink-$suffix.apk
    else
      IFS=';' read -r -a abi_array <<< "$ABIS"
      for ABI in "${abi_array[@]}"
      do
        PER_ABI_APK=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix.apk
        PER_ABI_APK_UNSIGNED=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix-unsigned.apk

        if [ -f "$PER_ABI_APK" ]; then
          # Standard per-ABI APK (Qt < 6.7 behaviour)
          mv -u "$PER_ABI_APK" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
        elif [ -f "$PER_ABI_APK_UNSIGNED" ] && [ -v FDROID ]; then
          # Unsigned APK (no signing key configured)
          mv -u "$PER_ABI_APK_UNSIGNED" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
        else
          # Try a broader find: ABI in filename OR in directory path (Qt 6.3+ sub-projects)
          FOUND=$(find "$OUT_APP_DIR" -type f \( \
                    -name "*${ABI}*${suffix}*.apk" -o \
                    \( -name "*${suffix}*.apk" -path "*${ABI}*" \) \
                  \) 2>/dev/null | head -1)
          if [ -n "$FOUND" ]; then
            if [[ ! -v FDROID && "$FOUND" == *"unsigned.apk" ]]; then
              echo "ERROR: Found only an unsigned APK for ABI=$ABI in release mode: $FOUND"
              exit 1
            fi
            mv -u "$FOUND" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
          else
            if [ -f "$PER_ABI_APK_UNSIGNED" ] || [ -f "$UNIVERSAL_APK_UNSIGNED" ]; then
              echo "ERROR: Only unsigned APK artifacts were produced for ABI=$ABI in release mode."
              echo "Check ANDROID_RELEASE_KEYSTORE_* secrets and signing configuration."
            else
              echo "ERROR: APK not found for ABI=$ABI (tried $PER_ABI_APK and $UNIVERSAL_APK_SIGNED)"
            fi
            echo "Contents of $APK_OUT_DIR:"
            ls -la "$APK_OUT_DIR" 2>/dev/null || echo "(directory does not exist)"
            echo "All APKs under android-build:"
            find "$ANDROID_BUILD_OUT_DIR" -name "*.apk" 2>/dev/null || true
            exit 1
          fi
        fi
      done
    fi
  fi
fi
