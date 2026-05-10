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
 -t, --tv                         Build Android TV UI variant APK/AAB artifact
 -h, --help                       Display this help

EOT
}

BUILD_TYPE="release"
TV_BUILD=0
DEFAULT_ANDROID_ABIS="armeabi-v7a;arm64-v8a"
SUPPORTED_ANDROID_ABIS="x86;x86_64;armeabi-v7a;arm64-v8a"

opts=$(getopt -l debug,aab,apk:,build-platform:,move,fdroid,tv,help -o "dua:b:mfth" -- "$@")
eval set -- "$opts"
while true; do
  case "$1" in
    -d | --debug) BUILD_TYPE="debug"; shift;;
    -u | --aab) AAB=1; shift;;
    -a | --apk) ABIS=$2; shift 2;;
    -b | --build-platform) ANDROID_BUILD_PLATFORM=$2; shift 2;;
    -m | --move) MOVE_RESULT=1; shift;;
    -f | --fdroid) FDROID=1; shift;;
    -t | --tv) TV_BUILD=1; shift;;
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
if [[ $TV_BUILD -eq 1 ]]; then
  build_suffix="tv-$build_suffix"
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

qt_android_dir_suffix_for_abi() {
  local abi=$1
  case "$abi" in
    "armeabi-v7a") echo "armv7";;
    "arm64-v8a") echo "arm64_v8a";;
    *) echo "$abi";;
  esac
}

resolve_available_android_abis() {
  local qt_host_path=$1
  local result=()
  IFS=';' read -r -a supported_abis <<< "$SUPPORTED_ANDROID_ABIS"

  for abi in "${supported_abis[@]}"
  do
    local qt_suffix
    qt_suffix=$(qt_android_dir_suffix_for_abi "$abi")
    local plugin_path="$qt_host_path/../android_${qt_suffix}/plugins/platforms/libplugins_platforms_qtforandroid_${abi}.so"
    if [[ -f "$plugin_path" ]]; then
      result+=("$abi")
    fi
  done

  if [[ ${#result[@]} -eq 0 ]]; then
    echo "$DEFAULT_ANDROID_ABIS"
    return 0
  fi

  local joined
  joined=$(IFS=';'; echo "${result[*]}")
  echo "$joined"
}

stage_missing_qt_platform_plugins() {
  local android_build_out_dir=$1
  local abi_list=$2

  if [[ -z "$abi_list" ]]; then
    return 0
  fi

  IFS=';' read -r -a abi_array <<< "$abi_list"
  for abi in "${abi_array[@]}"
  do
    [[ -z "$abi" ]] && continue

    local dest_dir="$android_build_out_dir/libs/$abi"
    local dest_file="$dest_dir/libplugins_platforms_qtforandroid_${abi}.so"

    if [[ -f "$dest_file" ]]; then
      continue
    fi

    local qt_suffix
    qt_suffix=$(qt_android_dir_suffix_for_abi "$abi")
    local src_file="$QT_HOST_PATH/../android_${qt_suffix}/plugins/platforms/libplugins_platforms_qtforandroid_${abi}.so"

    if [[ ! -f "$src_file" ]]; then
      echo "ERROR: Qt platform plugin source not found for ABI=$abi: $src_file"
      return 1
    fi

    mkdir -p "$dest_dir"
    cp "$src_file" "$dest_file"
    echo "Staged missing Qt platform plugin for $abi: $src_file -> $dest_file"
  done
}

stage_missing_qt_runtime_plugins() {
  local android_build_out_dir=$1
  local abi_list=$2

  if [[ -z "$abi_list" ]]; then
    return 0
  fi

  IFS=';' read -r -a abi_array <<< "$abi_list"
  for abi in "${abi_array[@]}"
  do
    [[ -z "$abi" ]] && continue

    local qt_suffix
    qt_suffix=$(qt_android_dir_suffix_for_abi "$abi")
    local qt_android_root="$QT_HOST_PATH/../android_${qt_suffix}"
    local dest_dir="$android_build_out_dir/libs/$abi"
    mkdir -p "$dest_dir"

    # Format: "<relative plugin path>|<required:1/0>"
    local plugin_specs=(
      "plugins/imageformats/libplugins_imageformats_qsvg_${abi}.so|1"
      "plugins/iconengines/libplugins_iconengines_qsvgicon_${abi}.so|1"
      "plugins/tls/libplugins_tls_qopensslbackend_${abi}.so|1"
      "plugins/tls/libplugins_tls_qcertonlybackend_${abi}.so|0"
    )

    for spec in "${plugin_specs[@]}"
    do
      local rel="${spec%%|*}"
      local required="${spec##*|}"
      local src="$qt_android_root/$rel"
      local base
      base=$(basename "$rel")
      local dst="$dest_dir/$base"

      if [[ -f "$dst" ]]; then
        continue
      fi

      if [[ -f "$src" ]]; then
        cp "$src" "$dst"
        echo "Staged Qt runtime plugin for $abi: $src -> $dst"
      elif [[ "$required" = "1" ]]; then
        echo "ERROR: Required Qt runtime plugin not found for ABI=$abi: $src"
        return 1
      else
        echo "WARN: Optional Qt runtime plugin not found for ABI=$abi: $src"
      fi
    done
  done
}

stage_android_third_party_libs() {
  local android_build_out_dir=$1
  local abi_list=$2
  local project_dir=$3

  if [[ -z "$abi_list" ]]; then
    return 0
  fi

  local prebuilt_root="$project_dir/client/3rd-prebuilt/3rd-prebuilt"
  IFS=';' read -r -a abi_array <<< "$abi_list"
  for abi in "${abi_array[@]}"
  do
    [[ -z "$abi" ]] && continue

    local dest_dir="$android_build_out_dir/libs/$abi"
    mkdir -p "$dest_dir"

    local lib_specs=(
      "amneziawg/android/$abi/libwg-go.so|1"
      "openvpn/android/$abi/libck-ovpn-plugin.so|1"
      "openvpn/android/$abi/libovpn3.so|1"
      "openvpn/android/$abi/libovpnutil.so|1"
      "openvpn/android/$abi/librsapss.so|1"
      "openssl/android/$abi/libcrypto_3.so|1"
      "openssl/android/$abi/libssl_3.so|1"
      "libssh/android/$abi/libssh.so|1"
    )

    for spec in "${lib_specs[@]}"
    do
      local rel="${spec%%|*}"
      local required="${spec##*|}"
      local src="$prebuilt_root/$rel"
      local dst="$dest_dir/$(basename "$rel")"

      if [[ -f "$dst" ]]; then
        continue
      fi

      if [[ -f "$src" ]]; then
        cp "$src" "$dst"
        echo "Staged Android third-party lib for $abi: $src -> $dst"
      elif [[ "$required" = "1" ]]; then
        echo "ERROR: Required Android third-party lib not found for ABI=$abi: $src"
        return 1
      else
        echo "WARN: Optional Android third-party lib not found for ABI=$abi: $src"
      fi
    done
  done
}

stage_missing_gamepad_qml_libs() {
  local android_build_out_dir=$1
  local abi_list=$2
  local build_dir=$3

  if [[ -z "$abi_list" ]]; then
    return 0
  fi

  IFS=';' read -r -a abi_array <<< "$abi_list"
  for abi in "${abi_array[@]}"
  do
    [[ -z "$abi" ]] && continue

    local dest_dir="$android_build_out_dir/libs/$abi"
    local dest_file="$dest_dir/libGamepadLegacyQuickPrivate_${abi}.so"
    local abi_underscores="${abi//-/_}"

    if [[ -f "$dest_file" ]]; then
      continue
    fi

    mkdir -p "$dest_dir"

    local src_file=""
    for candidate in \
      "$build_dir/client/3rd/qtgamepad/src/imports/gamepad/libGamepadLegacyQuickPrivate_${abi}.so" \
      "$build_dir/client/3rd/qtgamepad/src/imports/gamepad/libGamepadLegacyQuickPrivate_${abi_underscores}.so" \
      "$build_dir/3rd/qtgamepad/src/imports/gamepad/libGamepadLegacyQuickPrivate_${abi}.so" \
      "$build_dir/3rd/qtgamepad/src/imports/gamepad/libGamepadLegacyQuickPrivate_${abi_underscores}.so"
    do
      if [[ -f "$candidate" ]]; then
        src_file="$candidate"
        break
      fi
    done

    if [[ -z "$src_file" ]]; then
      src_file=$(find "$build_dir" -type f \( \
          -name "libGamepadLegacyQuickPrivate_${abi}.so" -o \
          -name "libGamepadLegacyQuickPrivate_${abi_underscores}.so" \
        \) 2>/dev/null | head -1 || true)
    fi

    if [[ -n "$src_file" && -f "$src_file" ]]; then
      cp "$src_file" "$dest_file"
      echo "Staged missing gamepad QML lib for $abi: $src_file -> $dest_file"
    else
      echo "WARN: Gamepad QML lib not found for ABI=$abi (libGamepadLegacyQuickPrivate)."
    fi
  done
}

verify_apk_contains_qt_platform_plugin() {
  local apk_path=$1
  local abi_list=$2

  if [[ ! -f "$apk_path" ]]; then
    echo "ERROR: APK not found for verification: $apk_path"
    return 1
  fi

  local py_bin=""
  if command -v python3 >/dev/null 2>&1; then
    py_bin="python3"
  elif command -v python >/dev/null 2>&1; then
    py_bin="python"
  else
    echo "WARN: python not found; skip APK plugin verification for $apk_path"
    return 0
  fi

  "$py_bin" - "$apk_path" "$abi_list" <<'PY'
import sys
import zipfile

apk_path = sys.argv[1]
requested_abis = [x for x in sys.argv[2].split(";") if x]

with zipfile.ZipFile(apk_path, "r") as zf:
    names = set(zf.namelist())

detected_abis = set()
for entry in names:
    if not entry.startswith("lib/"):
        continue
    parts = entry.split("/")
    if len(parts) < 3:
        continue
    abi = parts[1]
    if abi:
        detected_abis.add(abi)

if not detected_abis:
    print(
        "WARN: APK contains no native libs under lib/<abi>/; "
        f"skip Qt platform plugin verification: {apk_path}"
    )
    sys.exit(0)

missing = []
abis_to_check = sorted(detected_abis)
if requested_abis:
    requested_set = set(requested_abis)
    unexpected_abis = sorted(detected_abis - requested_set)
    if unexpected_abis:
        print(
            "ERROR: APK contains native libraries for ABI(s) that were not requested: "
            f"{', '.join(unexpected_abis)}. "
            "This can make Android choose an ABI without the matching Qt application binary."
        )
        print(f"APK: {apk_path}")
        print(f"Requested ABIs: {', '.join(requested_abis)}")
        print(f"Detected ABIs in APK: {', '.join(sorted(detected_abis))}")
        sys.exit(1)
    abis_to_check = sorted(requested_set & detected_abis)
    if not abis_to_check:
        print(
            "WARN: None of the requested ABIs are present in APK; "
            f"requested={', '.join(requested_abis)}, detected={', '.join(sorted(detected_abis))}. "
            "Skip Qt platform plugin verification."
        )
        sys.exit(0)

for abi in abis_to_check:
    abi_underscores = abi.replace("-", "_")
    app_lib_candidates = [
        f"lib/{abi}/libFBLink_{abi}.so",
        f"lib/{abi}/libFBLink_{abi_underscores}.so",
        f"lib/{abi}/libFBLink.so",
    ]
    if not any(candidate in names for candidate in app_lib_candidates):
        missing.append(" or ".join(app_lib_candidates))

    expected_templates = [
        "lib/{abi}/libQt6Core_{abi}.so",
        "lib/{abi}/libQt6Gui_{abi}.so",
        "lib/{abi}/libQt6Qml_{abi}.so",
        "lib/{abi}/libQt6Quick_{abi}.so",
        "lib/{abi}/libQt6Network_{abi}.so",
        "lib/{abi}/libplugins_platforms_qtforandroid_{abi}.so",
        "lib/{abi}/libplugins_imageformats_qsvg_{abi}.so",
        "lib/{abi}/libplugins_iconengines_qsvgicon_{abi}.so",
        "lib/{abi}/libplugins_tls_qopensslbackend_{abi}.so",
    ]
    for tpl in expected_templates:
        expected = tpl.format(abi=abi)
        if expected not in names:
            missing.append(expected)

    required_third_party_libs = [
        "libwg-go.so",
        "libck-ovpn-plugin.so",
        "libovpn3.so",
        "libovpnutil.so",
        "librsapss.so",
        "libcrypto_3.so",
        "libssl_3.so",
        "libssh.so",
    ]
    for lib_name in required_third_party_libs:
        expected = f"lib/{abi}/{lib_name}"
        if expected not in names:
            missing.append(expected)

if missing:
    print(f"ERROR: APK is missing required native runtime library/plugin(s): {', '.join(missing)}")
    print(f"APK: {apk_path}")
    print(f"Detected ABIs in APK: {', '.join(sorted(detected_abis))}")
    if requested_abis:
        print(f"Requested ABIs: {', '.join(requested_abis)}")
    sys.exit(1)

print(
    f"Verified Qt platform plugin in APK: {apk_path} "
    f"(ABIs checked: {', '.join(abis_to_check)})"
)
PY
}

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"

if [[ -v AAB && -z "${ABIS:-}" ]]; then
  ABIS="$DEFAULT_ANDROID_ABIS"
fi

if [[ "${ABIS:-}" = "all" ]]; then
  ABIS=$(resolve_available_android_abis "$QT_HOST_PATH")
  echo "Resolved 'all' Android ABIs to installed Qt targets: $ABIS"
fi

ANDROID_ABIS_FOR_PACKAGING="${ABIS:-}"
CONFIGURE_ALL_ABIS=0
if [[ -n "${ABIS:-}" && "${ABIS:-}" == *";"* ]]; then
  CONFIGURE_ALL_ABIS=1
fi

# Determine path to qt bin folder with qt-cmake
if [[ $CONFIGURE_ALL_ABIS -eq 1 ]]; then
  # For multi-ABI builds use qt-cmake from the first requested ABI
  # instead of hardcoding x86_64 (allows CI without x86_64 Qt target).
  if [[ "$ABIS" = *";"* ]]; then
    first_abi=$(echo "$ABIS" | cut -d';' -f 1)
  else
    first_abi="$ABIS"
  fi
  qt_bin_dir_suffix=$(qt_android_dir_suffix_for_abi "$first_abi")
else
  if [[ $ABIS = *";"* ]]; then
    oneOf=$(echo $ABIS | cut -d';' -f 1)
  else
    oneOf=$ABIS
  fi
  qt_bin_dir_suffix=$(qt_android_dir_suffix_for_abi "$oneOf")
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

qt_cmake_opts+=(-DQT_ANDROID_ABIS="$ABIS")

# QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON - Skip building apks as part of the default 'ALL' target
# We'll build apks during androiddeployqt
$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
  -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
  -DQT_USE_TARGET_ANDROID_BUILD_DIR=ON \
  -DFBLINK_ANDROID_TV=$([[ $TV_BUILD -eq 1 ]] && echo ON || echo OFF) \
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
# Ensure Qt platform plugins are present for all selected ABIs before Gradle.
stage_missing_qt_platform_plugins "$ANDROID_BUILD_OUT_DIR" "${ANDROID_ABIS_FOR_PACKAGING:-}"
# Ensure required Qt runtime plugins (SVG/TLS/icon engine) are present.
stage_missing_qt_runtime_plugins "$ANDROID_BUILD_OUT_DIR" "${ANDROID_ABIS_FOR_PACKAGING:-}"
# Ensure native protocol/runtime dependencies are present for every packaged ABI.
stage_android_third_party_libs "$ANDROID_BUILD_OUT_DIR" "${ANDROID_ABIS_FOR_PACKAGING:-}" "$PROJECT_DIR"
# Ensure vendored QtGamepad QML runtime lib is present when gamepad support is enabled.
stage_missing_gamepad_qml_libs "$ANDROID_BUILD_OUT_DIR" "${ANDROID_ABIS_FOR_PACKAGING:-}" "$BUILD_DIR"

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

if [ -v ABIS ]; then
  APK_VERIFY_DIR="$ANDROID_BUILD_OUT_DIR/build/outputs/apk/$BUILD_TYPE"
  if [[ ! -d "$APK_VERIFY_DIR" ]]; then
    echo "ERROR: APK output directory not found: $APK_VERIFY_DIR"
    exit 1
  fi

  mapfile -t BUILT_APKS < <(find "$APK_VERIFY_DIR" -maxdepth 1 -type f -name "*.apk" 2>/dev/null)
  if [[ ${#BUILT_APKS[@]} -eq 0 ]]; then
    echo "ERROR: No APK files found in $APK_VERIFY_DIR"
    exit 1
  fi

  for APK_FILE in "${BUILT_APKS[@]}"
  do
    verify_apk_contains_qt_platform_plugin "$APK_FILE" "$ABIS"
  done
fi

if [[ -v CI || -v MOVE_RESULT ]]; then
  echo "Moving APK/AAB..."
  if [ -v AAB ]; then
    AAB_FILE=$(find "$ANDROID_BUILD_OUT_DIR" -type f -name "*.aab" 2>/dev/null | head -1)
    if [ -z "$AAB_FILE" ]; then
      echo "ERROR: AAB not found under $ANDROID_BUILD_OUT_DIR"
      exit 1
    fi
    mv -u "$AAB_FILE" $ARTIFACT_DIR/FBLink-$BUILD_TYPE.aab
    if [[ $TV_BUILD -eq 1 ]]; then
      mv -u $ARTIFACT_DIR/FBLink-$BUILD_TYPE.aab $ARTIFACT_DIR/FBLinkTV-$BUILD_TYPE.aab
    fi
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
      if [[ $TV_BUILD -eq 1 ]]; then
        mv -u "$UNIVERSAL_APK_SIGNED" $ARTIFACT_DIR/FBLinkTV-$suffix.apk
      else
        mv -u "$UNIVERSAL_APK_SIGNED" $ARTIFACT_DIR/FBLink-$suffix.apk
      fi
    elif [ -f "$UNIVERSAL_APK_UNSIGNED" ] && [ -v FDROID ]; then
      if [[ $TV_BUILD -eq 1 ]]; then
        mv -u "$UNIVERSAL_APK_UNSIGNED" $ARTIFACT_DIR/FBLinkTV-$suffix.apk
      else
        mv -u "$UNIVERSAL_APK_UNSIGNED" $ARTIFACT_DIR/FBLink-$suffix.apk
      fi
    else
      IFS=';' read -r -a abi_array <<< "$ABIS"
      for ABI in "${abi_array[@]}"
      do
        PER_ABI_APK=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix.apk
        PER_ABI_APK_UNSIGNED=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix-unsigned.apk

        if [ -f "$PER_ABI_APK" ]; then
          # Standard per-ABI APK (Qt < 6.7 behaviour)
          if [[ $TV_BUILD -eq 1 ]]; then
            mv -u "$PER_ABI_APK" $ARTIFACT_DIR/FBLinkTV-$suffix.apk
          else
            mv -u "$PER_ABI_APK" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
          fi
        elif [ -f "$PER_ABI_APK_UNSIGNED" ] && [ -v FDROID ]; then
          # Unsigned APK (no signing key configured)
          if [[ $TV_BUILD -eq 1 ]]; then
            mv -u "$PER_ABI_APK_UNSIGNED" $ARTIFACT_DIR/FBLinkTV-$suffix.apk
          else
            mv -u "$PER_ABI_APK_UNSIGNED" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
          fi
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
            if [[ $TV_BUILD -eq 1 ]]; then
              mv -u "$FOUND" $ARTIFACT_DIR/FBLinkTV-$suffix.apk
            else
              mv -u "$FOUND" $ARTIFACT_DIR/FBLink-$ABI-$suffix.apk
            fi
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

if [[ -v MOVE_RESULT || -v CI ]]; then
  if [[ -d "$BUILD_DIR" ]]; then
    echo "Cleaning temporary Android build workspace: $BUILD_DIR"
    rm -rf "$BUILD_DIR"
  fi
fi
