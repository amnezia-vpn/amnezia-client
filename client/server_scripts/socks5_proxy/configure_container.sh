#!/bin/sh

echo -e "#!/bin/3proxy" > /usr/local/3proxy/conf/3proxy.cfg
echo -e "config /usr/local/3proxy/conf/3proxy.cfg" >> /usr/local/3proxy/conf/3proxy.cfg
echo -e "timeouts 1 5 30 60 180 1800 15 60" >> /usr/local/3proxy/conf/3proxy.cfg

echo -e "$SOCKS5_USER" >> /usr/local/3proxy/conf/3proxy.cfg

echo -e "log /usr/local/3proxy/logs/3proxy.log" >> /usr/local/3proxy/conf/3proxy.cfg
echo -e "logformat \"-\\\"\"+_G{\"\"time_unix\"\":%t, \"\"proxy\"\":{\"\"type:\"\":\"\"%N\"\", \"\"port\"\":%p}, \"\"error\"\":{\"\"code\"\":\"\"%E\"\"}, \"\"auth\"\":{\"\"user\"\":\"\"%U\"\"}, \"\"client\"\":{\"\"ip\"\":\"\"%C\"\", \"\"port\"\":%c}, \"\"server\"\":{\"\"ip\"\":\"\"%R\"\", \"\"port\"\":%r}, \"\"bytes\"\":{\"\"sent\"\":%O, \"\"received\"\":%I}, \"\"request\"\":{\"\"hostname\"\":\"\"%n\"\"}, \"\"message\"\":\"\"%T\"\"}\"" >> /usr/local/3proxy/conf/3proxy.cfg
echo -e "auth $SOCKS5_AUTH_TYPE" >> /usr/local/3proxy/conf/3proxy.cfg
echo -e "socks -p$SOCKS5_PROXY_PORT" >> /usr/local/3proxy/conf/3proxy.cfg