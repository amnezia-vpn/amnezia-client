sudo docker run \
    --restart=always \
    -p 500:500/udp \
    -p 4500:4500/udp \
    -d --privileged \
    --name $CONTAINER_NAME $CONTAINER_NAME

sudo docker network connect amnezia-dns-net $CONTAINER_NAME
