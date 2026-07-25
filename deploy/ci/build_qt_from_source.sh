#!/bin/bash
# Build Qt from source for Ubuntu 20.04 (glibc 2.31).
# Official Qt Linux binaries require glibc >= 2.34 and will not run on 20.04.
set -euo pipefail

: "${QT_VERSION:=6.10.1}"
: "${QT_PREFIX:=/opt/Qt/${QT_VERSION}/gcc_64}"
: "${QT_SRC_DIR:=/opt/qt-src}"
: "${QT_BUILD_DIR:=/opt/qt-build}"
: "${CMAKE_BUILD_PARALLEL_LEVEL:=$(nproc)}"

export CMAKE_BUILD_PARALLEL_LEVEL

MODULES=(
  qtbase
  qtdeclarative
  qtremoteobjects
  qt5compat
  qtshadertools
  qtsvg
  qttools
  qtimageformats
  qttranslations
)

if [[ -x "${QT_PREFIX}/bin/qmake" ]] || [[ -x "${QT_PREFIX}/bin/qt-cmake" ]]; then
  echo "Qt ${QT_VERSION} already present at ${QT_PREFIX}"
  exit 0
fi

echo "Building Qt ${QT_VERSION} from source into ${QT_PREFIX}..."
rm -rf "${QT_SRC_DIR}" "${QT_BUILD_DIR}"
mkdir -p "${QT_SRC_DIR}" "${QT_BUILD_DIR}"

git clone --branch "v${QT_VERSION}" --depth 1 \
  https://code.qt.io/qt/qt5.git "${QT_SRC_DIR}"

cd "${QT_SRC_DIR}"
# init-repository is the supported way to fetch required modules.
./init-repository --module-subset="$(IFS=,; echo "${MODULES[*]}")" --no-resolve-deps

mkdir -p "${QT_BUILD_DIR}"
cd "${QT_BUILD_DIR}"

cmake -G Ninja \
  -DCMAKE_INSTALL_PREFIX="${QT_PREFIX}" \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=ON \
  -DQT_BUILD_EXAMPLES=OFF \
  -DQT_BUILD_TESTS=OFF \
  "${QT_SRC_DIR}"

cmake --build . --parallel "${CMAKE_BUILD_PARALLEL_LEVEL}"
cmake --install .

# Minimal layout expected by deploy/build.sh (Tools/CMake may be missing).
mkdir -p "$(dirname "${QT_PREFIX}")/../Tools"
ln -sfn /usr/local "$(dirname "${QT_PREFIX}")/../Tools/CMake" || true

echo "Qt ${QT_VERSION} installed to ${QT_PREFIX}"
"${QT_PREFIX}/bin/qmake" -query QT_VERSION || "${QT_PREFIX}/libexec/qmake" -query QT_VERSION || true
