CUR_USER=$(whoami 2>/dev/null || echo $HOME | sed 's/.*\///');\
sudo mkdir -p $DOCKERFILE_FOLDER;\
sudo chown $CUR_USER $DOCKERFILE_FOLDER;\
if ! sudo docker network ls | grep -q amnezia-dns-net; then sudo docker network create \
  --driver bridge \
  --subnet=172.29.172.0/24 \
  --opt com.docker.network.bridge.name=amn0 \
  amnezia-dns-net;\
fi

if ! grep -q "#!/bin/bash" /opt/amnezia/setup_host_firewall.sh; then
  sudo sed -i '1i\#!/bin/bash\n' /opt/amnezia/setup_host_firewall.sh
fi

if lsmod | grep -qw nf_tables; then
    sudo update-alternatives --set iptables /usr/sbin/iptables-nft
    sudo cat > /etc/systemd/system/setup-host-firewall.service << EOF
[Unit]
Description=Run setup_host_firewall.sh
PartOf=nftables.service
After=nftables.service

[Service]
Type=oneshot
ExecStart=/opt/amnezia/setup_host_firewall.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF
else
  sudo cat > /etc/systemd/system/setup-host-firewall.service << EOF
[Unit]
Description=Run setup_host_firewall.sh

[Service]
Type=oneshot
ExecStart=/opt/amnezia/setup_host_firewall.sh
RemainAfterExit=yes

[Install]
WantedBy=multi-user.target
EOF
fi

sudo systemctl enable setup-host-firewall.service