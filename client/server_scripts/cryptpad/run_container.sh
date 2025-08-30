sudo docker run -d \
--log-driver none \
--restart always \
-p $CRYPTPAD_PORT:3000/tcp \
--name $CONTAINER_NAME \
$CONTAINER_NAME
