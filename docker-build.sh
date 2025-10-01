#!/bin/bash
set -e

echo "=== AmneziaVPN ARM64 Docker Build ==="

# Colors
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Build Docker image if needed
echo -e "${BLUE}Building Docker image...${NC}"
docker build --platform linux/arm64 -t amnezia-build-arm64 -f Dockerfile.arm64 .

# Run build in container
echo -e "${BLUE}Starting build container...${NC}"
docker run --platform linux/arm64 --rm -it \
    -w /workspace \
    -v "$(pwd):/workspace" \
    -e QT_BIN_DIR=/opt/Qt/6.7.3/gcc_arm64/bin \
    -e QIF_BIN_DIR=/opt/Qt/Tools/QtInstallerFramework/4.7/bin \
    amnezia-build-arm64 \
    bash -c "
        cd /workspace &&
        echo '${GREEN}Running build script...${NC}' &&
        bash deploy/build_linux.sh
    "

echo -e "${GREEN}Build completed!${NC}"
echo "Artifacts are in: deploy/"
