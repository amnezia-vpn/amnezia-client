#!/bin/bash
# shellcheck disable=SC2086

set -o errexit -o nounset

usage() {
  cat <<EOT

Usage:
  build_android [options] <artifact_types>

Build AmneziaVPN android client.

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
BUILD_DIR=$DEPLOY_DIR/build
OUT_APP_DIR=$BUILD_DIR/client

echo "Project dir: $PROJECT_DIR"
echo "Build dir: $BUILD_DIR"

# Determine path to qt bin folder with qt-cmake
if [[ -v AAB || "$ABIS" = "all" ]]; then
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

# Run qt-cmake to configure build
qt_cmake_opts=()

if [[ -v AAB || "$ABIS" = "all" ]]; then
  qt_cmake_opts+=(-DQT_ANDROID_BUILD_ALL_ABIS=ON)
else
  qt_cmake_opts+=(-DQT_ANDROID_ABIS="$ABIS")
fi

# QT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON - Skip building apks as part of the default 'ALL' target
# We'll build apks during androiddeployqt
$QT_BIN_DIR/qt-cmake -S $PROJECT_DIR -B $BUILD_DIR \
  -DQT_NO_GLOBAL_APK_TARGET_PART_OF_ALL=ON \
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

if [ -v ANDROID_BUILD_PLATFORM ]; then
  deployqt_opts+=(--android-platform "$ANDROID_BUILD_PLATFORM")
fi

if [ "$BUILD_TYPE" = "release" ]; then
  deployqt_opts+=(--release)
fi

# for gradle to skip all tasks when it is executed by androiddeployqt
# gradle is started later explicitly
export ANDROIDDEPLOYQT_RUN=1

$QT_HOST_PATH/bin/androiddeployqt \
  --input $OUT_APP_DIR/android-FBLink-deployment-settings.json \
  --output $OUT_APP_DIR/android-build \
  "${deployqt_opts[@]}"

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

$OUT_APP_DIR/android-build/gradlew \
  --project-dir $OUT_APP_DIR/android-build \
  -DexplicitRun=1 \
  "${gradle_opts[@]}"

if [[ -v CI || -v MOVE_RESULT ]]; then
  echo "Moving APK/AAB..."
  if [ -v AAB ]; then
    AAB_FILE=$OUT_APP_DIR/android-build/build/outputs/bundle/$BUILD_TYPE/AmneziaVPN-$BUILD_TYPE.aab
    if [ ! -f "$AAB_FILE" ]; then
      AAB_FILE=$(find "$OUT_APP_DIR/android-build" -type f -name "*.aab" 2>/dev/null | head -1)
    fi
    mv -u "$AAB_FILE" $PROJECT_DIR/deploy/build/AmneziaVPN-$BUILD_TYPE.aab
  fi

  if [ -v ABIS ]; then
    if [ "$ABIS" = "all" ]; then
      ABIS="x86;x86_64;armeabi-v7a;arm64-v8a"
    fi

    suffix=$BUILD_TYPE
    if [ -v FDROID ]; then
      suffix+="-unsigned"
    fi

    APK_OUT_DIR=$OUT_APP_DIR/android-build/build/outputs/apk/$BUILD_TYPE

    # Qt 6.7+ with QT_ANDROID_BUILD_ALL_ABIS=ON produces a single universal APK
    # instead of separate per-ABI files.  Detect which naming convention was used.
    UNIVERSAL_APK=$APK_OUT_DIR/AmneziaVPN-$suffix.apk
    UNIVERSAL_APK_UNSIGNED=$APK_OUT_DIR/AmneziaVPN-$suffix-unsigned.apk
    # resolve: prefer signed, fall back to unsigned
    if [ ! -f "$UNIVERSAL_APK" ] && [ -f "$UNIVERSAL_APK_UNSIGNED" ]; then
      UNIVERSAL_APK=$UNIVERSAL_APK_UNSIGNED
    fi

    IFS=';' read -r -a abi_array <<< "$ABIS"
    for ABI in "${abi_array[@]}"
    do
      PER_ABI_APK=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix.apk
      PER_ABI_APK_UNSIGNED=$APK_OUT_DIR/AmneziaVPN-$ABI-$suffix-unsigned.apk

      if [ -f "$PER_ABI_APK" ]; then
        # Standard per-ABI APK (Qt < 6.7 behaviour)
        mv -u "$PER_ABI_APK" $PROJECT_DIR/deploy/build/
      elif [ -f "$PER_ABI_APK_UNSIGNED" ]; then
        # Unsigned APK (no signing key configured)
        mv -u "$PER_ABI_APK_UNSIGNED" $PROJECT_DIR/deploy/build/AmneziaVPN-$ABI-$suffix.apk
      else
        # Try a broader find: ABI in filename OR in directory path (Qt 6.3+ sub-projects)
        FOUND=$(find "$OUT_APP_DIR" -type f \( \
                  -name "*${ABI}*${suffix}*.apk" -o \
                  \( -name "*${suffix}*.apk" -path "*${ABI}*" \) \
                \) 2>/dev/null | head -1)
        if [ -n "$FOUND" ]; then
          mv -u "$FOUND" $PROJECT_DIR/deploy/build/AmneziaVPN-$ABI-$suffix.apk
        elif [ -f "$UNIVERSAL_APK" ]; then
          # Qt 6.7+ universal APK: copy it once per requested ABI so the
          # downstream rename/upload steps keep working as before.
          cp "$UNIVERSAL_APK" $PROJECT_DIR/deploy/build/AmneziaVPN-$ABI-$suffix.apk
        else
          echo "ERROR: APK not found for ABI=$ABI (tried $PER_ABI_APK and $UNIVERSAL_APK)"
          echo "Contents of $APK_OUT_DIR:"
          ls -la "$APK_OUT_DIR" 2>/dev/null || echo "(directory does not exist)"
          echo "All APKs under android-build:"
          find "$OUT_APP_DIR/android-build" -name "*.apk" 2>/dev/null || true
          exit 1
        fi
      fi
    done
  fi
fi
