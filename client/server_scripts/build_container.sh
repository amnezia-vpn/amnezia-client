if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then sudo sh -c "\
  test -d /var/cache/containers || mkdir -m 700 -p /var/cache/containers;\
  test -f /var/cache/containers/short-name-aliases.conf || chmod 600 /var/cache/containers/short-name-aliases.conf>>/var/cache/containers/short-name-aliases.conf;\
  grep -q '\[aliases\]' /var/cache/containers/short-name-aliases.conf || echo '[aliases]' >> /var/cache/containers/short-name-aliases.conf;\
  grep -q '  # Amnezia start' /var/cache/containers/short-name-aliases.conf || printf '%s\n' \
    '  # Amnezia start' \
    '  \"3proxy/3proxy\" = \"docker.io/3proxy/3proxy\"' \
    '  \"amneziavpn/amnezia-wg\" = \"docker.io/amneziavpn/amnezia-wg\"' \
    '  \"amneziavpn/amneziawg-go\" = \"docker.io/amneziavpn/amneziawg-go\"' \
    '  \"amneziavpn/ipsec-server\" = \"docker.io/amneziavpn/ipsec-server\"' \
    '  \"amneziavpn/torpress\" = \"docker.io/amneziavpn/torpress\"' \
    '  \"atmoz/sftp\" = \"docker.io/atmoz/sftp\"' \
    '  \"mvance/unbound\" = \"docker.io/mvance/unbound\"' \
    '  \"alpine\" = \"docker.io/library/alpine\"' \
    '  # Amnezia finish' \
    >> /var/cache/containers/short-name-aliases.conf";\
fi;\
sudo docker build --no-cache --pull -t $CONTAINER_NAME $DOCKERFILE_FOLDER
