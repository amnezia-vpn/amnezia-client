sudo docker stop $CONTAINER_NAME;\
sudo docker rm -fv $CONTAINER_NAME;\
sudo docker rmi $CONTAINER_NAME
if sudo docker volume ls | grep -q $CONTAINER_NAME; then
    sudo docker volume rm -f $CONTAINER_NAME
fi
