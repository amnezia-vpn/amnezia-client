if [ -n "$(docker --version 2>/dev/null | grep 'podman')" ]; then \
  if ! sudo test -d "/var/cache/containers"; then \
    sudo mkdir -m 700 -p /var/cache/containers; fi;\
  if ! sudo test -f "/var/cache/containers/short-name-aliases.conf"; then \
    sudo touch /var/cache/containers/short-name-aliases.conf;\
    sudo chmod 600 /var/cache/containers/short-name-aliases.conf; fi;\
  if ! sudo grep -q '\[aliases\]' /var/cache/containers/short-name-aliases.conf; then \
    sudo sh -c "echo '[aliases]' >> /var/cache/containers/short-name-aliases.conf"; fi;\
  if ! sudo grep -q '  \# Amnezia start' /var/cache/containers/short-name-aliases.conf; then \
    sudo sh -c 'cat >> /var/cache/containers/short-name-aliases.conf \
<< EOF
  # Amnezia start
  "3proxy/3proxy" = "docker.io/3proxy/3proxy"
  "amneziavpn/amnezia-wg" = "docker.io/amneziavpn/amnezia-wg"
  "amneziavpn/amneziawg-go" = "docker.io/amneziavpn/amneziawg-go"
  "amneziavpn/ipsec-server" = "docker.io/amneziavpn/ipsec-server"
  "amneziavpn/torpress" = "docker.io/amneziavpn/torpress"
  "atmoz/sftp" = "docker.io/atmoz/sftp"
  "mvance/unbound" = "docker.io/mvance/unbound"
  "alpine" = "docker.io/library/alpine"
  # Amnezia finish
EOF';\
  fi;\
fi;\
sudo docker build --no-cache --pull -t $CONTAINER_NAME $DOCKERFILE_FOLDER
