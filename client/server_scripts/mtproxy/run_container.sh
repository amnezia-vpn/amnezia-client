# Run container
MT_PORT="$MTPROXY_PORT"; MT_SECRET="$MTPROXY_SECRET"; MT_TAG="$MTPROXY_TAG"
[ -n "$MT_PORT" ] || MT_PORT="443"
if [ -z "$MT_SECRET" ]; then
    MT_SECRET="$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n' | tr 'A-F' 'a-f')"
fi
sudo mkdir -p "$DOCKERFILE_FOLDER/data"
TAG_ARG=""
if [ -n "$MT_TAG" ]; then TAG_ARG="-e TAG=$MT_TAG"; fi
sudo docker run -d \
    --log-driver none \
    --restart always \
    -p "$MT_PORT:443/tcp" \
    -v "$DOCKERFILE_FOLDER/data:/data" \
    -e "SECRET=$MT_SECRET" \
    $TAG_ARG \
    --name "$CONTAINER_NAME" \
    "$CONTAINER_NAME"
