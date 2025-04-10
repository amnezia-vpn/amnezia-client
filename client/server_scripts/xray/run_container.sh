# Run container
sudo docker run -d \
--privileged \
--log-driver none \
--restart always \
--cap-add=NET_ADMIN \
-p $XRAY_SERVER_PORT:$XRAY_SERVER_PORT/tcp \
--name $CONTAINER_NAME $CONTAINER_NAME

sudo docker network connect amnezia-dns-net $CONTAINER_NAME

# Create tun device if not exist
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /dev/net; if [ ! -c /dev/net/tun ]; then mknod /dev/net/tun c 10 200; fi'

# Prevent to route packets outside of the container in case if server behind of the NAT
#sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"

# Create service for podman
if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then \
  sudo sh -c "podman generate systemd --new --name $CONTAINER_NAME 2>/dev/null > $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service";\
  sudo cp $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service /etc/systemd/system/;\
  sudo systemctl daemon-reload && sudo systemctl enable --now container-$CONTAINER_NAME.service && sudo docker update --restart no $CONTAINER_NAME > /dev/null;\
fi
