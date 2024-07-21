sudo systemctl disable --now container-$CONTAINER_NAME.service;\
sudo docker stop $CONTAINER_NAME;\
sudo docker rm -fv $CONTAINER_NAME && sudo rm -f $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service;\
sudo docker rmi $CONTAINER_NAME
