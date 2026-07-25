#!/bin/bash
# Install Qt Installer Framework that can run on the current glibc.
# Official recent IFW builds may require a newer glibc than Ubuntu 20.04 provides.
set -euo pipefail

: "${QIF_VERSION:=4.7}"
: "${QIF_ROOT:=/opt/Qt/Tools/QtInstallerFramework/${QIF_VERSION}}"

if [[ -x "${QIF_ROOT}/bin/binarycreator" ]]; then
  if "${QIF_ROOT}/bin/binarycreator" --help >/dev/null 2>&1; then
    echo "Qt IFW ${QIF_VERSION} already usable at ${QIF_ROOT}"
    exit 0
  fi
fi

echo "Installing Qt IFW ${QIF_VERSION} via aqt..."
python3 -m pip install --upgrade "aqtinstall==3.3.0" "py7zr==0.22.*"
python3 -m aqt install-tool linux desktop tools_ifw "4.${QIF_VERSION#4.}" -O /opt/Qt \
  || python3 -m aqt install-tool linux desktop tools_ifw "${QIF_VERSION}" -O /opt/Qt \
  || true

# aqt layout varies; normalize to /opt/Qt/Tools/QtInstallerFramework/<ver>
if [[ ! -x "${QIF_ROOT}/bin/binarycreator" ]]; then
  found="$(find /opt/Qt -type f -name binarycreator 2>/dev/null | head -1 || true)"
  if [[ -n "${found}" ]]; then
    mkdir -p "${QIF_ROOT}/bin"
    ln -sfn "$(dirname "${found}")/"* "${QIF_ROOT}/bin/" || cp -a "$(dirname "${found}")/." "${QIF_ROOT}/bin/"
  fi
fi

if [[ ! -x "${QIF_ROOT}/bin/binarycreator" ]]; then
  echo "ERROR: binarycreator not found after IFW install" >&2
  exit 1
fi

if ! "${QIF_ROOT}/bin/binarycreator" --help >/dev/null 2>&1; then
  echo "ERROR: binarycreator does not run on this glibc. Need an older IFW or build IFW from source." >&2
  ldd "${QIF_ROOT}/bin/binarycreator" || true
  exit 1
fi

echo "Qt IFW ready: ${QIF_ROOT}"
