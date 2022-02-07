# Run container
sudo docker stop amnezia-tor
sudo docker rm amnezia-tor
sudo docker run -d -p 80:80 --restart always --name $CONTAINER_NAME tutum/wordpress
sudo docker run -d --link $CONTAINER_NAME --name amnezia-tor goldy/tor-hidden-service
sudo docker exec -i amnezia-tor apk add bash
