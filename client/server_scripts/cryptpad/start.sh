
#!/bin/bash
# CryptPad start script
# This script starts the CryptPad container

CONTAINER_NAME="amnezia-cryptpad"
CRYPTPAD_PORT="${CRYPTPAD_PORT:-3000}"

# Check if container is already running
if docker ps --format '{{.Names}}' | grep -q "^${CONTAINER_NAME}$"; then
    echo "Container ${CONTAINER_NAME} is already running"
    exit 0
fi

# Start the container
docker start ${CONTAINER_NAME}

if [ $? -eq 0 ]; then
    echo "CryptPad container started successfully"
    echo "Access CryptPad at: http://$(hostname -I | awk '{print $1}'):${CRYPTPAD_PORT}"
else
    echo "Failed to start CryptPad container"
    exit 1
fi
