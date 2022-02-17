# From: https://git.sr.ht/~cofob/docker-images/tree/master/item/dns/alfis/preloaded/Dockerfile
# Git cloning
FROM		debian:bullseye-slim AS code-cloner

WORKDIR		/src
RUN			apt-get update && apt-get install -y git 
RUN			git clone https://github.com/Revertron/Alfis


# Blockchain loading
FROM		debian:bullseye-slim AS blockchain

WORKDIR		/
RUN			apt-get update && apt-get install -y wget
RUN			wget -O blockchain.db https://alfis.cofob.ru/blockchain.db



# Compiling
FROM		rust:slim-buster AS builder

RUN			apt-get update && apt-get install pkg-config libcairo2-dev -y
COPY		--from=code-cloner /src/Alfis /src
WORKDIR		/src
RUN			cargo build --release --no-default-features --features="doh"


# Compressing binary
FROM		gruebel/upx:latest as upx
COPY		--from=builder /src/target/release/alfis /app.fat
RUN			upx --best --lzma -o /alfis /app.fat


# Bare debian
FROM		debian:bullseye-slim AS sys


# Final image
FROM		scratch

LABEL		maintainer="AmneziaVPN"

COPY		--from=upx /alfis /alfis
COPY		--from=sys /lib /lib
COPY		--from=sys /lib64 /lib64
COPY		alfis.toml /storage/alfis.toml
COPY		--from=blockchain /blockchain.db /storage/blockchain.db

EXPOSE		53/tcp
EXPOSE		53/udp

WORKDIR		/storage

VOLUME		[ "/storage" ]

CMD			[ "/alfis", "-c", "/storage/alfis.toml" ]