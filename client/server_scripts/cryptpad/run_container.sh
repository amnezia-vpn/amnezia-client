#!/bin/bash

CONTAINER_NAME="amnezia-cryptpad"
IMAGE_NAME="amnezia-cryptpad" # This should match the image name built by CMake

# Stop and remove any existing container with the same name
sudo docker stop $CONTAINER_NAME || true
sudo docker rm $CONTAINER_NAME || true

# Run the CryptPad container in detached mode
sudo docker run -d \
--log-driver none \
--restart always \
--name $CONTAINER_NAME \
-p 3000:3000 \
-v cryptpad_data:/cryptpad/data \
-v cryptpad_config:/cryptpad/config \
$IMAGE_NAME

# Wait for the container to start and the .onion address to be generated
ONION_ADDRESS=""
for i in $(seq 1 60); do # Increased timeout to 60 seconds
    if sudo docker exec $CONTAINER_NAME test -f "/var/lib/tor/cryptpad_hidden_service/hostname"; then
        ONION_ADDRESS=$(sudo docker exec $CONTAINER_NAME cat "/var/lib/tor/cryptpad_hidden_service/hostname")
        echo "Generated Tor Hidden Service Address: $ONION_ADDRESS"
        break
    fi
    sleep 1
done

if [ -z "$ONION_ADDRESS" ]; then
    echo "Error: Failed to get Tor Hidden Service Address from container." >&2
    exit 1
fi
