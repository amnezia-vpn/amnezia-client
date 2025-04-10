# Run container
sudo docker run -d \
--log-driver none \
--restart always \
--privileged \
--cap-add=NET_ADMIN \
--cap-add=SYS_MODULE \
-p $AWG_SERVER_PORT:$AWG_SERVER_PORT/udp \
-v /lib/modules:/lib/modules \
--sysctl="net.ipv4.conf.all.src_valid_mark=1" \
--name $CONTAINER_NAME \
$CONTAINER_NAME

sudo docker network connect amnezia-dns-net $CONTAINER_NAME

# Prevent to route packets outside of the container in case if server behind of the NAT
#sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"

# Create service for podman
if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then \
  sudo sh -c "podman generate systemd --restart-policy=always -t 10 --name $CONTAINER_NAME 2>/dev/null > $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service";\
  sudo cp $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service /etc/systemd/system/;\
  sudo systemctl daemon-reload && sudo systemctl enable --now container-$CONTAINER_NAME.service && sudo docker update --restart no $CONTAINER_NAME > /dev/null;\
fi
