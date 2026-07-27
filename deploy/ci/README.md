# Linux CI build (Ubuntu 22.04 container)

The Linux installer is produced by the **`Build-Linux-Ubuntu`** job, which now
builds inside an `ubuntu:22.04` container (glibc 2.35) so the binary runs on
Ubuntu 22.04 and newer.

| Job / Artifact | Built in | Runs on |
|---|---|---|
| `Build-Linux-Ubuntu` → `AmneziaVPN_*_linux_x64.run` | `ubuntu:22.04` (glibc 2.35) | Ubuntu 22.04+ |

## Why build in a container

Official Qt 6.10 Linux binaries need glibc ≥ 2.34, and building on a host with a
newer glibc produces a binary that will not start on Ubuntu 22.04. Building
inside an `ubuntu:22.04` container caps the glibc baseline at 2.35, and all Qt
libraries (including `QtRemoteObjects`, required for IPC) are bundled into the
package via the CMake install rules.

Conan C/C++ dependencies (OpenSSL, etc.) are always compiled inside the same
container (`CONAN_INSTALL_ARGS=--build=*`) so they are never reused from remote
binaries linked against a newer glibc.

## Local reproduction

```bash
docker run --rm -it -v "$PWD":/src -w /src ubuntu:22.04 bash -lc '
  apt-get update && apt-get install -y git ca-certificates
  bash deploy/ci/linux_container_setup.sh
  pip3 install conan==2.28.0 aqtinstall==3.3.0 "py7zr==0.22.*"
  # ... install Qt with aqt, then:
  CONAN_INSTALL_ARGS="--build=*;-o=openssl/*:shared=True" \
    QT_INSTALL_DIR=/opt deploy/build.sh --generator Ninja --installer all -f
'
```

Helpers live in `deploy/ci/`.
