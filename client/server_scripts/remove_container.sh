sudo docker stop $CONTAINER_NAME;\
sudo docker rm -fv $CONTAINER_NAME;\
sudo docker rmi $CONTAINER_NAME;\
sudo rm -frd /opt/amnezia/$CONTAINER_NAME; sudo rmdir /opt/amnezia 2>/dev/null
