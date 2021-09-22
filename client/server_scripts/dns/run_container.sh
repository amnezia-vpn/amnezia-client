# Run container
sudo docker run -d --restart always --cap-add=NET_ADMIN -p 53:53/udp -p 53:53/tcp --name $CONTAINER_NAME $CONTAINER_NAME
