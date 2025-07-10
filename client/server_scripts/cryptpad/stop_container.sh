#!/bin/bash

CONTAINER_NAME="amnezia-cryptpad"

sudo docker stop $CONTAINER_NAME || true
sudo docker rm $CONTAINER_NAME || true
