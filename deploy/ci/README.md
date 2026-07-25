# Linux CI builds for Ubuntu 20.04 / 22.04 (compat) + default Build-Linux-Ubuntu

Default Linux installer is still produced by **`Build-Linux-Ubuntu`** on
`android-runner` (unchanged).

Additionally, **`Build-Linux-Ubuntu-Compat`** builds installers inside matching
Ubuntu containers so binaries do not require a newer glibc than the target OS.

| Job / Artifact | Built in | Runs on |
|---|---|---|
| `Build-Linux-Ubuntu` → `AmneziaVPN_*_linux_x64.run` | `android-runner` | newer glibc hosts |
| `AmneziaVPN_*_linux_x64_ubuntu20.04.run` | `ubuntu:20.04` (glibc 2.31) | Ubuntu 20.04+ |
| `AmneziaVPN_*_linux_x64_ubuntu22.04.run` | `ubuntu:22.04` (glibc 2.35) | Ubuntu 22.04+ |

## Why compat jobs

Official Qt 6.10 Linux binaries need glibc ≥ 2.34, so they cannot be shipped
for Ubuntu 20.04. The 20.04 job therefore builds Qt from source inside the
container (cached between runs). The 22.04 job uses official Qt packages via
`aqt`.

Conan C/C++ dependencies (OpenSSL, etc.) are always compiled inside the same
container (`CONAN_INSTALL_ARGS=--build=*`) for compat jobs.

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
