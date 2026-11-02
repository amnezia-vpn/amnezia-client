sudo docker run -d \
  --log-driver none \
  --restart always \
  -p $WHATSAPP_PROXY_CHAT_PORT:443/tcp \
  -p $WHATSAPP_PROXY_MEDIA_PORT:587/tcp \
  -v amnezia-whatsappproxy-data:/data \
  --name $CONTAINER_NAME \
  $CONTAINER_NAME
