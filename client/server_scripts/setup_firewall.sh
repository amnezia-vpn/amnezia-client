sysctl -w net.ipv4.ip_forward=1
iptables -P FORWARD ACCEPT
iptables -A INPUT -p icmp --icmp-type echo-request -j DROP
