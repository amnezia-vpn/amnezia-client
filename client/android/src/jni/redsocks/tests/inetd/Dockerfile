FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y xinetd

RUN set -o xtrace \
    && apt-get install -y iperf

COPY testing /etc/xinetd.d/testing

CMD ["/usr/sbin/xinetd", "-dontfork", "-pidfile", "/run/xinetd.pid", "-inetd_ipv6"]
