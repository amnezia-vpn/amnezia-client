FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y nginx-light

RUN set -o xtrace \
    && dd if=/dev/urandom of=/usr/share/nginx/html/128K count=1 bs=128K \
    && dd if=/dev/urandom of=/usr/share/nginx/html/1M count=1 bs=1M \
    && for i in `seq 16`; do cat /usr/share/nginx/html/1M; done >/usr/share/nginx/html/16M

CMD ["/usr/sbin/nginx", "-g", "daemon off;"]
