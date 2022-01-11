# Git cloning
FROM		debian:bullseye-slim AS code-cloner

WORKDIR		/src

RUN			apt-get update && apt-get install -y git 
RUN			git clone https://github.com/yggdrasil-network/yggdrasil-go


# Compiling
FROM		golang:1.16-alpine AS builder
ENV			CGO_ENABLED 0
ENV			TZ=Europe/Moscow

COPY		--from=code-cloner /src /src
WORKDIR		/src/yggdrasil-go
RUN			./build && rm -rf /root /go
RUN			/src/yggdrasil-go/yggdrasil -genconf > /config


# Compressing binary
FROM		gruebel/upx:latest as upx
COPY		--from=builder /src/yggdrasil-go/yggdrasil /app.fat
COPY		--from=builder /src/yggdrasil-go/yggdrasilctl /appctl.fat
RUN			upx --best --lzma -o /yggdrasil /app.fat
RUN			upx --best --lzma -o /yggdrasilctl /appctl.fat


# Final image
FROM		alpine

COPY		--from=upx /yggdrasil /yggdrasil
COPY		--from=upx /yggdrasilctl /yggdrasilctl
COPY		--from=builder /config /config/yggdrasil.conf
COPY		--from=builder /var /var
COPY		--from=builder /run /run
COPY		entrypoint.sh /entrypoint.sh

VOLUME		[ "/config" ]

ENTRYPOINT	[ "sh", "/entrypoint.sh" ]
