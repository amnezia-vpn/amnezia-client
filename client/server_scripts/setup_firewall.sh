sudo sysctl -w net.ipv4.ip_forward=1
sudo iptables -P FORWARD ACCEPT
sudo iptables -A INPUT -p icmp --icmp-type echo-request -j DROP
