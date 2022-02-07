# Run container
sudo docker run -d --restart always --network amnezia-dns-net --ip=172.29.172.254 --name $CONTAINER_NAME $CONTAINER_NAME
