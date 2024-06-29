if [ -n "$(docker --version 2>/dev/null | grep 'podman')" ]; then \
  AliDir=/var/cache/containers;\
  AliFile=short-name-aliases.conf;\
  if [ ! -d "$AliDir" ]; then \
    sudo mkdir -m 700 -p $AliDir;\
  fi;\
  if [ ! -f "$AliDir/$AliFile" ]; then \
    sudo echo '[aliases]' >> $AliDir/$AliFile;\
    sudo chmod 600 $AliDir/$AliFile;\
  fi;\
  if ! sudo grep -q '  # AmneziaVPN' $AliDir/$AliFile; then sudo echo -e \
    '  # Amnezia start\n  "3proxy/3proxy" = "docker.io/3proxy/3proxy"\n  "amneziavpn/amnezia-wg" = "docker.io/amneziavpn/amnezia-wg"\n  "amneziavpn/amneziawg-go" = "docker.io/amneziavpn/amneziawg-go"\n  "amneziavpn/ipsec-server" = "docker.io/amneziavpn/ipsec-server"\n  "amneziavpn/torpress" = "docker.io/amneziavpn/torpress"\n  "atmoz/sftp" = "docker.io/atmoz/sftp"\n  "mvance/unbound" = "docker.io/mvance/unbound"\n  "alpine" = "docker.io/library/alpine"\n  # Amnezia finish' \
    >> $AliDir/$AliFile;\
  fi;\
fi;\
sudo docker build --no-cache --pull -t $CONTAINER_NAME $DOCKERFILE_FOLDER
