# Run container
sudo docker run -d \
-p $JITSI_HTTPS_PORT:8443 \
--name $CONTAINER_NAME $CONTAINER_NAME
