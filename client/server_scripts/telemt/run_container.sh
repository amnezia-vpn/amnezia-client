# Run container (ulimit per Telemt docs — avoids "Too many open files" under load)
sudo docker run -d \
  --log-driver none \
  --restart always \
  --ulimit nofile=65536:65536 \
  -p $TELEMT_PORT:$TELEMT_PORT/tcp \
  -v amnezia-telemt-data:/data \
  --name $CONTAINER_NAME \
  $CONTAINER_NAME
