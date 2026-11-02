#!/bin/sh

if [ ! -f /data/haproxy.cfg ] || [ ! -s /data/proxy.pem ]; then
    echo "ERROR: /data/haproxy.cfg not found — run configure_container first"
    tail -f /dev/null
    exit 1
fi

exec haproxy -W -db -f /data/haproxy.cfg
