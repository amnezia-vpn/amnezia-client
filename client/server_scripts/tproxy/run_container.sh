# Run Telegram WEB proxy (host ports map to Caddy 80/443 inside the container)
sudo docker run -d \
  --log-driver none \
  --restart always \
  -p $TPROXY_HTTP_PORT:80/tcp \
  -p $TPROXY_HTTPS_PORT:443/tcp \
  -v amnezia-tproxy-data:/data \
  --name $CONTAINER_NAME \
  $CONTAINER_NAME
