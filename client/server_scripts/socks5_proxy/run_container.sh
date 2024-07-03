sudo docker run -d \
--restart always \
-p $SOCKS5_PROXY_PORT:$SOCKS5_PROXY_PORT/tcp \
--name $CONTAINER_NAME \
$CONTAINER_NAME
