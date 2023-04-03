#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# kill daemons in case of restart
killall -KILL ssserver

# start daemons if configured
if [ -f /opt/amnezia/shadowsocks/ss-config.json ]; then (ssserver -c /opt/amnezia/shadowsocks/ss-config.json &); fi

tail -f /dev/null
