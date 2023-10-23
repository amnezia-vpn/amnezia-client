sudo docker build --no-cache --pull -t $CONTAINER_NAME $DOCKERFILE_FOLDER --build-arg SERVER_ARCH=$(uname -m)
