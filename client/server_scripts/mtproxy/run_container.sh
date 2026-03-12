# Run container
sudo docker run -d \
  --log-driver none \
  --restart always \
  -p $MTPROXY_PORT:443/tcp \
  -v amnezia-mtproxy-data:/data \
  --name $CONTAINER_NAME \
  $CONTAINER_NAME
