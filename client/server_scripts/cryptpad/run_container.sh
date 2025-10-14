sudo docker run -d \
--log-driver none \
--restart always \
-p $CRYPTPAD_PORT:3000/tcp \
--name $CONTAINER_NAME \
$CONTAINER_NAME

# Ensure host firewall allows external access to CryptPad port
sudo iptables -C INPUT -p tcp --dport $CRYPTPAD_PORT -j ACCEPT || \
sudo iptables -A INPUT -p tcp --dport $CRYPTPAD_PORT -j ACCEPT
