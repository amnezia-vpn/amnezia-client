# syntax=docker/dockerfile:1
# Telegram WEB proxy: Caddy + tproxy-server + official MTProxy.
# Official MTProxy is x86_64-only (same constraint as upstream install.sh).

FROM golang:1.22-bookworm AS relay
WORKDIR /src
ARG TPROXY_COMMIT=2873a08806d6e4d84830b9b5c4b0ec0f46af91f8
RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends ca-certificates curl git && rm -rf /var/lib/apt/lists/*; \
    curl -fL --retry 5 --retry-delay 3 --connect-timeout 10 --max-time 120 \
        -o /tmp/tproxy.tar.gz \
        "https://github.com/telegramdesktop/tproxy-server/archive/${TPROXY_COMMIT}.tar.gz"; \
    tar -xzf /tmp/tproxy.tar.gz -C /src --strip-components=1; \
    go build -trimpath -o /out/tproxy-server ./cmd/tproxy-server

FROM debian:12-slim AS mtproxy
ARG MTPROXY_COMMIT=f36d8af769ffaeac36978d38c2c0f6d1104c2137
ARG MTPROXY_SHA256=919795c416b870670841a21d1930ad97a24c7b84b9eb8c6f9e3de32f2fdf4655
RUN set -eux; \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl build-essential libssl-dev zlib1g-dev make && \
    rm -rf /var/lib/apt/lists/*; \
    curl -fL --retry 5 --retry-delay 3 --connect-timeout 10 --max-time 120 \
        -o /tmp/MTProxy.tar.gz \
        "https://github.com/TelegramMessenger/MTProxy/archive/${MTPROXY_COMMIT}.tar.gz"; \
    echo "${MTPROXY_SHA256}  /tmp/MTProxy.tar.gz" | sha256sum -c -; \
    mkdir -p /src && tar -xzf /tmp/MTProxy.tar.gz -C /src --strip-components=1; \
    make -C /src -j"$(nproc)"; \
    test -x /src/objs/bin/mtproto-proxy; \
    install -m 0755 /src/objs/bin/mtproto-proxy /mtproto-proxy

FROM debian:12-slim
RUN set -eux; \
    ARCH="$(uname -m)"; \
    if [ "$ARCH" != "x86_64" ]; then \
        echo "tproxy-server requires an x86_64 server (official MTProxy is x86_64-only)" >&2; \
        exit 1; \
    fi; \
    apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates curl openssl libssl3 zlib1g && \
    rm -rf /var/lib/apt/lists/*; \
    curl -fL --retry 5 --retry-delay 3 --connect-timeout 10 --max-time 120 \
        -o /tmp/caddy.tgz \
        "https://github.com/caddyserver/caddy/releases/download/v2.8.4/caddy_2.8.4_linux_amd64.tar.gz"; \
    tar -xzf /tmp/caddy.tgz -C /usr/local/bin caddy; \
    chmod 0755 /usr/local/bin/caddy; \
    rm -f /tmp/caddy.tgz

COPY --from=relay /out/tproxy-server /usr/local/bin/tproxy-server
COPY --from=mtproxy /mtproto-proxy /usr/local/bin/mtproto-proxy

RUN mkdir -p /opt/amnezia /data /data/site /data/caddy \
 && printf '#!/bin/sh\ntail -f /dev/null\n' > /opt/amnezia/start.sh \
 && chmod a+x /opt/amnezia/start.sh /usr/local/bin/tproxy-server /usr/local/bin/mtproto-proxy

VOLUME /data
ENTRYPOINT ["/bin/sh", "/opt/amnezia/start.sh"]
CMD [""]
