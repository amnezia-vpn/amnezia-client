sudo docker stop $CONTAINER_NAME;\
if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then \
  sudo systemctl disable --now container-$CONTAINER_NAME.service;\
  sudo systemctl daemon-reload; sudo systemctl reset-failed;\
  sudo rm -f /etc/systemd/system/container-$CONTAINER_NAME.service;\
  sudo systemctl daemon-reload;\
fi;\
sudo docker rm -fv $CONTAINER_NAME;\
sudo docker rmi $CONTAINER_NAME
