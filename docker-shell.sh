#!/bin/bash
set -e

echo "=== Starting ARM64 Docker Shell ==="

# Build Docker image if needed
docker build --platform linux/arm64 -t amnezia-build-arm64 -f Dockerfile.arm64 .

# Start interactive shell
docker run --platform linux/arm64 --rm -it \
    -w /workspace \
    -v "$(pwd):/workspace" \
    -e QT_BIN_DIR=/opt/Qt/6.7.3/gcc_arm64/bin \
    -e QIF_BIN_DIR=/opt/Qt/Tools/QtInstallerFramework/4.7/bin \
    amnezia-build-arm64 \
    /bin/bash
