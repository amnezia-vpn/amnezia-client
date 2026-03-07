FROM alpine:3.15
LABEL maintainer="AmneziaVPN"

ARG XRAY_RELEASE="v25.8.3"

ENV TZ=Asia/Shanghai

RUN apk add --no-cache --update curl unzip bash openssl netcat-openbsd dumb-init rng-tools xz \
 && mkdir -p /opt/amnezia/xray /etc/security \
 && printf '#!/bin/bash\ntail -f /dev/null\n' > /opt/amnezia/start.sh \
 && chmod +x /opt/amnezia/start.sh \
 && curl -fsSL "https://github.com/XTLS/Xray-core/releases/download/${XRAY_RELEASE}/Xray-linux-64.zip" \
      -o /tmp/xray.zip \
 && unzip /tmp/xray.zip xray -d /usr/bin/ \
 && chmod +x /usr/bin/xray \
 && rm -rf /tmp/* /var/cache/apk/* /var/tmp/* /usr/share/terminfo \
 && cat >> /etc/sysctl.conf <<'EOF'
fs.file-max = 51200
net.core.rmem_max = 67108864
net.core.wmem_max = 67108864
net.core.netdev_max_backlog = 250000
net.core.somaxconn = 4096
net.core.default_qdisc = fq
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
net.ipv4.tcp_congestion_control = bbr
EOF
 && cat >> /etc/security/limits.conf <<'EOF'
* soft nofile 51200
* hard nofile 51200
EOF

ENTRYPOINT ["dumb-init", "/opt/amnezia/start.sh"]
