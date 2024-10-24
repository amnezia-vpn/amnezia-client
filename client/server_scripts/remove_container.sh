sudo docker stop $CONTAINER_NAME;\
sudo docker --version 2>/dev/null | grep -q podman && \
  sudo systemctl disable --now container-$CONTAINER_NAME.service && \
  sudo systemctl daemon-reload && sudo systemctl reset-failed && \
  sudo rm -f $DOCKERFILE_FOLDER/container-$CONTAINER_NAME.service;\
sudo docker rm -fv $CONTAINER_NAME;\
sudo docker rmi $CONTAINER_NAME
