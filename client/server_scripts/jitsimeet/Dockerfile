# Inspired by https://github.com/jitsi/docker-jitsi-meet/blob/master/docker-compose.yml

FROM jitsi/web:unstable as jitsi-web
FROM jitsi/prosody:unstable as jitsi-prosody
FROM jitsi/jicofo:unstable as jitsi-jicofo
FROM jitsi/jvb:unstable as jitsi-jvb

ENV JVB_AUTH_USER=admin
ENV JICOFO_AUTH_PASSWORD=admin
ENV JVB_AUTH_PASSWORD=admin
ENV CONFIG=.
ENV HTTP_PORT=80
ENV HTTPS_PORT=443

LABEL maintainer="AmneziaVPN"
