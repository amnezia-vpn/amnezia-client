FROM ubuntu:14.04

RUN set -o xtrace \
    && sed -i 's,^deb-src,# no src # &,; s,http://archive.ubuntu.com/ubuntu/,mirror://mirrors.ubuntu.com/mirrors.txt,' /etc/apt/sources.list \
    && apt-get update \
    && apt-get install -y apache2-utils curl pv python-tornado

RUN set -o xtrace \
    && apt-get install -y iperf

COPY benchmark.py /benchmark.py

# yandex-tank is nice `ab' replacement, but it's PPA is broken at the moment
# https://github.com/yandex/yandex-tank/issues/202
#
# apt-get install -y software-properties-common
# add-apt-repository ppa:yandex-load/main
# apt-get install -y phantom phantom-ssl yandex-tank
