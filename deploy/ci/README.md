# Linux CI builds for Ubuntu 20.04 / 22.04

AmneziaVPN Linux installers are produced by GitHub Actions in matching Ubuntu
containers so the resulting binaries do not require a newer glibc than the
target OS provides.

| Artifact | Built in | Runs on |
|---|---|---|
| `AmneziaVPN_*_linux_x64_ubuntu20.04.run` | `ubuntu:20.04` (glibc 2.31) | Ubuntu 20.04+ |
| `AmneziaVPN_*_linux_x64_ubuntu22.04.run` | `ubuntu:22.04` (glibc 2.35) | Ubuntu 22.04+ |
| `AmneziaVPN_*_linux_x64_ubuntu24.04.run` | `ubuntu:24.04` (glibc 2.39) | Ubuntu 24.04+ |

## Why separate jobs

Official Qt 6.10 Linux binaries need glibc ≥ 2.34, so they cannot be shipped
for Ubuntu 20.04. The 20.04 job therefore builds Qt from source inside the
container (cached between runs). The 22.04/24.04 jobs use official Qt packages via
`aqt`.

Conan C/C++ dependencies (OpenSSL, etc.) are always compiled inside the same
container (`CONAN_INSTALL_ARGS=--build=*`). Reusing prebuilts built on Ubuntu
24.04 is what caused `GLIBC_2.38 not found` on 20.04/22.04.

## Local reproduction

```bash
# 22.04-compatible installer
docker run --rm -it -v "$PWD":/src -w /src ubuntu:22.04 bash -lc '
  apt-get update && apt-get install -y git ca-certificates
  bash deploy/ci/linux_container_setup.sh
  pip3 install conan==2.28.0 aqtinstall==3.3.0 "py7zr==0.22.*"
  # ... install Qt with aqt, then:
  CONAN_INSTALL_ARGS="--build=*;-o=openssl/*:shared=True" \
    QT_INSTALL_DIR=/opt deploy/build.sh --generator Ninja --installer all -f
'

# 20.04-compatible installer (builds Qt from source — slow first time)
docker run --rm -it -v "$PWD":/src -w /src ubuntu:20.04 bash -lc '
  apt-get update && apt-get install -y git ca-certificates
  bash deploy/ci/linux_container_setup.sh
  bash deploy/ci/build_qt_from_source.sh
  bash deploy/ci/install_qt_ifw.sh
  pip3 install conan==2.28.0
  CONAN_INSTALL_ARGS="--build=*;-o=openssl/*:shared=True" \
    QT_INSTALL_DIR=/opt deploy/build.sh --generator Ninja --installer all -f
'
```

Helpers live in `deploy/ci/`.
