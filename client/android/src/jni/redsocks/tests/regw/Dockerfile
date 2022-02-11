FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y libevent-2.0-5 valgrind curl strace

COPY redsocks       /usr/local/sbin/
COPY redsocks.conf  /usr/local/etc/
CMD ["/usr/local/sbin/redsocks", "-c", "/usr/local/etc/redsocks.conf"]
