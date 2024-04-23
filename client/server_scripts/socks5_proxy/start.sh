#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"

/proxy socks -t tcp -p "0.0.0.0:$SOCKS5_PROXY_PORT"
