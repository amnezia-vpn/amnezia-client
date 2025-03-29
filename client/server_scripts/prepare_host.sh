CUR_USER=$(whoami 2>/dev/null || echo $HOME | sed 's/.*\///');\
sudo mkdir -p $DOCKERFILE_FOLDER;\
sudo chown $CUR_USER $DOCKERFILE_FOLDER;\
if ! sudo docker network ls | grep -q amnezia-dns-net; then sudo docker network create \
  --driver bridge \
  --subnet=172.29.172.0/24 \
  --opt com.docker.network.bridge.name=amn0 \
  amnezia-dns-net;\
fi

# check if nf_tables is loaded
if lsmod | grep -qw nf_tables; then
    sudo update-alternatives --set iptables /usr/sbin/iptables-nft
fi