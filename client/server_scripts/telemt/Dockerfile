# syntax=docker/dockerfile:1
# Debian-based image with Telemt binary (shell + jq for Amnezia configure scripts).
# Binary from https://github.com/telemt/telemt releases (same pattern as upstream Dockerfile minimal stage).

FROM debian:12-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        binutils \
        ca-certificates \
        curl \
        jq \
        openssl \
        tar \
    && rm -rf /var/lib/apt/lists/*

# Use machine arch (works with classic `docker build`; TARGETARCH is only set with BuildKit).
RUN set -eux; \
    ARCH="$(uname -m)"; \
    case "$ARCH" in \
        x86_64) ASSET="telemt-x86_64-linux-musl.tar.gz" ;; \
        aarch64|arm64) ASSET="telemt-aarch64-linux-musl.tar.gz" ;; \
        *) echo "Unsupported architecture: $ARCH" >&2; exit 1 ;; \
    esac; \
    curl -fL --retry 5 --retry-delay 3 --connect-timeout 10 --max-time 120 \
        -o "/tmp/${ASSET}" "https://github.com/telemt/telemt/releases/latest/download/${ASSET}"; \
    curl -fL --retry 5 --retry-delay 3 --connect-timeout 10 --max-time 120 \
        -o "/tmp/${ASSET}.sha256" "https://github.com/telemt/telemt/releases/latest/download/${ASSET}.sha256"; \
    cd /tmp && sha256sum -c "${ASSET}.sha256"; \
    tar -xzf "${ASSET}" -C /tmp; \
    test -f /tmp/telemt; \
    install -m 0755 /tmp/telemt /usr/local/bin/telemt; \
    strip --strip-unneeded /usr/local/bin/telemt || true; \
    rm -f "/tmp/${ASSET}" "/tmp/${ASSET}.sha256" /tmp/telemt

RUN mkdir -p /opt/amnezia /data
RUN printf '#!/bin/sh\ntail -f /dev/null\n' > /opt/amnezia/start.sh && \
    chmod a+x /opt/amnezia/start.sh

VOLUME /data
ENTRYPOINT ["/bin/sh", "/opt/amnezia/start.sh"]
CMD [""]
