FROM alpine:3.15
LABEL maintainer="AmneziaVPN"

RUN apk add --no-cache --update curl wireguard-tools dumb-init \
 && mkdir -p /opt/amnezia /etc/security \
 && printf '#!/bin/bash\ntail -f /dev/null\n' > /opt/amnezia/start.sh \
 && chmod +x /opt/amnezia/start.sh \
 && rm -rf /var/cache/apk/* /tmp/* /var/tmp/* /usr/share/terminfo \
 && cat >> /etc/sysctl.conf <<'EOF'
fs.file-max = 51200
net.core.rmem_max = 67108864
net.core.wmem_max = 67108864
net.core.netdev_max_backlog = 250000
net.core.somaxconn = 4096
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_tw_recycle = 0
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 1200
net.ipv4.ip_local_port_range = 10000 65000
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.tcp_max_tw_buckets = 5000
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_mem = 25600 51200 102400
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864
net.ipv4.tcp_mtu_probing = 1
net.ipv4.tcp_congestion_control = hybla
# for low-latency network, use cubic instead
# net.ipv4.tcp_congestion_control = cubic
EOF
 && cat >> /etc/security/limits.conf <<'EOF'
* soft nofile 51200
* hard nofile 51200
EOF

ENTRYPOINT ["dumb-init", "/opt/amnezia/start.sh"]
