sudo docker stop $CONTAINER_NAME;\
sudo docker rm -fv $CONTAINER_NAME;\
sudo docker rmi $CONTAINER_NAME;\
test "$REMOVE_CONTAINER_DATA" = "1" && sudo docker volume rm -f ${CONTAINER_NAME}-data 2>/dev/null || true
