# Run container
sudo docker run -d --restart always -p 5300:53/udp -p 5300:53/tcp --name $CONTAINER_NAME $CONTAINER_NAME
