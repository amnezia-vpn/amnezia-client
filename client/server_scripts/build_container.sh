sudo docker build --network host -t $CONTAINER_NAME $DOCKERFILE_FOLDER --build-arg SERVER_ARCH=$(uname -m)
