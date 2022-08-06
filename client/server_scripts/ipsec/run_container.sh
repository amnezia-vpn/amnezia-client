sudo docker run -d \
--privileged \
--log-driver none \
--restart=always \
-p 500:500/udp \
-p 4500:4500/udp \
--name $CONTAINER_NAME $CONTAINER_NAME

sudo docker network connect amnezia-dns-net $CONTAINER_NAME
