#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

XRAY_PORT="$XRAY_SERVER_PORT"
if [ -z "$XRAY_PORT" ] && [ -f /opt/amnezia/xray/server.json ]; then
  XRAY_PORT=$(sed -n 's/.*"port":[[:space:]]*\([0-9]\+\).*/\1/p' /opt/amnezia/xray/server.json | head -n 1)
fi

ensure_iptables_rule() {
  local tool="$1"
  shift
  if command -v "$tool" >/dev/null 2>&1; then
    "$tool" -C INPUT "$@" >/dev/null 2>&1 || "$tool" -A INPUT "$@"
  fi
}

ensure_iptables_rule iptables -i lo -j ACCEPT
ensure_iptables_rule iptables -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
ensure_iptables_rule iptables -p icmp -j ACCEPT
ensure_iptables_rule iptables -p tcp --dport 80 -j ACCEPT
ensure_iptables_rule iptables -p tcp --dport 443 -j ACCEPT
if [ -n "$XRAY_PORT" ]; then ensure_iptables_rule iptables -p tcp --dport "$XRAY_PORT" -j ACCEPT; fi
command -v iptables >/dev/null 2>&1 && iptables -P INPUT DROP

ensure_iptables_rule ip6tables -i lo -j ACCEPT
ensure_iptables_rule ip6tables -m state --state RELATED,ESTABLISHED -j ACCEPT
ensure_iptables_rule ip6tables -p ipv6-icmp -j ACCEPT
if [ -n "$XRAY_PORT" ]; then ensure_iptables_rule ip6tables -p tcp --dport "$XRAY_PORT" -j ACCEPT; fi
command -v ip6tables >/dev/null 2>&1 && ip6tables -P INPUT DROP

# kill daemons in case of restart
killall -KILL xray 2>/dev/null || true

# start daemons if configured
if [ -f /opt/amnezia/xray/server.json ]; then
  exec xray -config /opt/amnezia/xray/server.json
fi

exec tail -f /dev/null
