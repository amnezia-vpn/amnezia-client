sudo docker run -d \
--log-driver none \
--restart always \
-p $SFTP_PORT:22/tcp \
--name $CONTAINER_NAME \
$CONTAINER_NAME \
$SFTP_USER:$SFTP_PASSWORD:::upload
