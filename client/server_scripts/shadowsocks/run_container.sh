# Run container
sudo docker run \
--log-driver none \
-d --restart always \
--cap-add=NET_ADMIN \
-p $SHADOWSOCKS_SERVER_PORT:$SHADOWSOCKS_SERVER_PORT/tcp \
-p $SHADOWSOCKS_SERVER_PORT:$SHADOWSOCKS_SERVER_PORT/udp \
--name $CONTAINER_NAME $CONTAINER_NAME

sudo docker network connect amnezia-dns-net $CONTAINER_NAME

# Prevent to route packets outside of the container in case if server behind of the NAT
sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"
