if [ -n "$(docker --version 2>/dev/null | grep 'podman')" ]; then \
  sudo sh -c "if [ ! -d '/var/cache/containers' ]; then \
    mkdir -m 700 -p /var/cache/containers; fi";\
  sudo sh -c "if [ ! -f '/var/cache/containers/short-name-aliases.conf' ]; then \
    touch /var/cache/containers/short-name-aliases.conf;\
    chmod 600 /var/cache/containers/short-name-aliases.conf; fi";\
  sudo sh -c "if ! grep -q '\[aliases\]' /var/cache/containers/short-name-aliases.conf; then \
    echo '[aliases]' >> /var/cache/containers/short-name-aliases.conf; fi";\
  sudo sh -c "if ! grep -q '  \# Amnezia start' /var/cache/containers/short-name-aliases.conf; then \
    cat >> /var/cache/containers/short-name-aliases.conf << EOF
  # Amnezia start
  \"3proxy/3proxy\" = \"docker.io/3proxy/3proxy\"
  \"amneziavpn/amnezia-wg\" = \"docker.io/amneziavpn/amnezia-wg\"
  \"amneziavpn/amneziawg-go\" = \"docker.io/amneziavpn/amneziawg-go\"
  \"amneziavpn/ipsec-server\" = \"docker.io/amneziavpn/ipsec-server\"
  \"amneziavpn/torpress\" = \"docker.io/amneziavpn/torpress\"
  \"atmoz/sftp\" = \"docker.io/atmoz/sftp\"
  \"mvance/unbound\" = \"docker.io/mvance/unbound\"
  \"alpine\" = \"docker.io/library/alpine\"
  # Amnezia finish
EOF
  fi";\
fi;\
sudo docker build --no-cache --pull -t $CONTAINER_NAME $DOCKERFILE_FOLDER
