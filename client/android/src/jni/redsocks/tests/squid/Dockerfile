FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y squid3

COPY squid-?.conf basic.passwd digest.passwd /etc/squid3/

# that's from /etc/init/squid3.conf
ENTRYPOINT ["/usr/sbin/squid3", "-NYC", "-f"]
CMD ["/etc/squid3/squid-8.conf"]
