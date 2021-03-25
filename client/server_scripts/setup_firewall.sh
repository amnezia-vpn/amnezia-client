sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -P FORWARD ACCEPT
sudo iptables -C INPUT -p icmp --icmp-type echo-request -j DROP || sudo iptables -A INPUT -p icmp --icmp-type echo-request -j DROP

# Tuning network
sudo sysctl fs.file-max=51200
sudo sysctl net.core.rmem_max=67108864
sudo sysctl net.core.wmem_max=67108864
sudo sysctl net.core.netdev_max_backlog=250000
sudo sysctl net.core.somaxconn=4096
sudo sysctl net.ipv4.tcp_syncookies=1
sudo sysctl net.ipv4.tcp_tw_reuse=1
sudo sysctl net.ipv4.tcp_tw_recycle=0
sudo sysctl net.ipv4.tcp_fin_timeout=30
sudo sysctl net.ipv4.tcp_keepalive_time=1200
sudo sysctl net.ipv4.ip_local_port_range="10000 65000"
sudo sysctl net.ipv4.tcp_max_syn_backlog=8192
sudo sysctl net.ipv4.tcp_max_tw_buckets=5000
sudo sysctl net.ipv4.tcp_fastopen=3
sudo sysctl net.ipv4.tcp_mem="25600 51200 102400"
sudo sysctl net.ipv4.tcp_rmem="4096 87380 67108864"
sudo sysctl net.ipv4.tcp_wmem="4096 65536 67108864"
sudo sysctl net.ipv4.tcp_mtu_probing=1
sudo sysctl net.ipv4.tcp_congestion_control=hybla
