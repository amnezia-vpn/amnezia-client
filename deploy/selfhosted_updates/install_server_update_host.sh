#!/bin/sh
set -eu

HOST_DIRECTORY="${1:-/opt/amnezia/client-updates}"
BRIDGE_HOST="${AMNEZIA_UPDATE_BRIDGE_HOST:-172.29.172.252}"
SYNC_PORT="${AMNEZIA_UPDATE_SYNC_PORT:-17865}"
CONTAINER_NAME="${AMNEZIA_UPDATE_CONTAINER_NAME:-amnezia-client-updates}"
HOST_CONTAINER_NAME="${AMNEZIA_UPDATE_HOST_CONTAINER_NAME:-${CONTAINER_NAME}-host}"
IMAGE="${AMNEZIA_UPDATE_IMAGE:-docker.io/library/busybox:1.36.1}"
VPN_CONTAINER="${AMNEZIA_UPDATE_VPN_CONTAINER:-}"
PUBLISH_HOST_PORT="${AMNEZIA_UPDATE_PUBLISH_HOST_PORT:-1}"
HOST_BIND="${AMNEZIA_UPDATE_HOST_BIND:-0.0.0.0}"
EXPECTED_SUBNET="172.29.172.0/24"
AUTO_VPN_CONTAINERS="amnezia-awg2 amnezia-awg amnezia-wireguard amnezia-openvpn"

die() {
    printf '%s\n' "$1" >&2
    exit 2
}

is_ipv4_address() {
    candidate="$1"
    case "$candidate" in
        ""|*/*)
            return 1
            ;;
    esac

    old_ifs="$IFS"
    IFS=.
    set -- $candidate
    IFS="$old_ifs"

    [ "$#" -eq 4 ] || return 1
    for octet do
        case "$octet" in
            ""|*[!0-9]*)
                return 1
                ;;
        esac
        [ "$octet" -ge 0 ] 2>/dev/null && [ "$octet" -le 255 ] 2>/dev/null || return 1
    done
}

is_port() {
    case "$1" in
        ""|*[!0-9]*)
            return 1
            ;;
    esac
    [ "$1" -ge 1 ] 2>/dev/null && [ "$1" -le 65535 ] 2>/dev/null
}

is_running_container() {
    sudo docker ps --format '{{.Names}}' | grep -qx "$1"
}

wait_http_ready() {
    container="$1"
    sudo docker exec "$container" sh -c "i=0; while [ \$i -lt 20 ]; do output=\$(busybox wget -S -O /dev/null 'http://127.0.0.1:$SYNC_PORT/' 2>&1 || true); printf '%s\n' \"\$output\" | grep -q 'HTTP/' && exit 0; i=\$((i + 1)); sleep 1; done; exit 1"
}

wait_host_http_ready() {
    if [ "$PUBLISH_HOST_PORT" != "1" ]; then
        return 0
    fi
    sudo docker run --rm \
        --log-driver none \
        --network host \
        --entrypoint sh \
        "$IMAGE" \
        -c "i=0; while [ \$i -lt 20 ]; do output=\$(busybox wget -S -O /dev/null 'http://127.0.0.1:$SYNC_PORT/' 2>&1 || true); printf '%s\n' \"\$output\" | grep -q 'HTTP/' && exit 0; i=\$((i + 1)); sleep 1; done; exit 1"
}

open_host_firewall_port() {
    if [ "$PUBLISH_HOST_PORT" != "1" ]; then
        return 0
    fi

    if command -v ufw >/dev/null 2>&1; then
        sudo ufw allow "${SYNC_PORT}/tcp" >/dev/null 2>&1 || true
    fi
    if command -v firewall-cmd >/dev/null 2>&1; then
        sudo firewall-cmd --add-port="${SYNC_PORT}/tcp" >/dev/null 2>&1 || true
        sudo firewall-cmd --runtime-to-permanent >/dev/null 2>&1 || true
    fi
    if command -v iptables >/dev/null 2>&1; then
        sudo iptables -C INPUT -p tcp --dport "$SYNC_PORT" -j ACCEPT >/dev/null 2>&1 \
            || sudo iptables -I INPUT -p tcp --dport "$SYNC_PORT" -j ACCEPT >/dev/null 2>&1 \
            || true
    fi
}

[ -n "$HOST_DIRECTORY" ] || die "HOST_DIRECTORY must not be empty"
case "$HOST_DIRECTORY" in
    /*)
        ;;
    *)
        die "HOST_DIRECTORY must be an absolute path"
        ;;
esac
[ -n "$CONTAINER_NAME" ] || die "AMNEZIA_UPDATE_CONTAINER_NAME must not be empty"
[ -n "$HOST_CONTAINER_NAME" ] || die "AMNEZIA_UPDATE_HOST_CONTAINER_NAME must not be empty"
is_ipv4_address "$BRIDGE_HOST" || die "AMNEZIA_UPDATE_BRIDGE_HOST must be a single IPv4 address, not a CIDR route"
is_ipv4_address "$HOST_BIND" || die "AMNEZIA_UPDATE_HOST_BIND must be a single IPv4 address"
is_port "$SYNC_PORT" || die "AMNEZIA_UPDATE_SYNC_PORT must be an integer from 1 to 65535"
case "$PUBLISH_HOST_PORT" in
    0|1)
        ;;
    *)
        die "AMNEZIA_UPDATE_PUBLISH_HOST_PORT must be 0 or 1"
        ;;
esac

sudo mkdir -p "$HOST_DIRECTORY/files"

NETWORK_NAME="amnezia-dns-net"
if ! sudo docker network inspect "$NETWORK_NAME" >/dev/null 2>&1; then
    sudo docker network create --driver bridge --subnet="$EXPECTED_SUBNET" --opt com.docker.network.bridge.name=amn0 "$NETWORK_NAME"
else
    NETWORK_SUBNETS="$(sudo docker network inspect -f '{{range .IPAM.Config}}{{println .Subnet}}{{end}}' "$NETWORK_NAME")"
    if ! printf '%s\n' "$NETWORK_SUBNETS" | grep -qx "$EXPECTED_SUBNET"; then
        NETWORK_NAME="${CONTAINER_NAME}-net"
        if ! sudo docker network inspect "$NETWORK_NAME" >/dev/null 2>&1; then
            sudo docker network create --driver bridge --subnet="$EXPECTED_SUBNET" "$NETWORK_NAME"
        fi
    fi
fi

if ! sudo docker image inspect "$IMAGE" >/dev/null 2>&1; then
    sudo docker pull "$IMAGE" >/dev/null
fi

VPN_CONTAINER_EXPLICIT=0
if [ -n "$VPN_CONTAINER" ]; then
    VPN_CONTAINER_EXPLICIT=1
else
    for candidate in $AUTO_VPN_CONTAINERS; do
        if is_running_container "$candidate"; then
            VPN_CONTAINER="$candidate"
            break
        fi
    done
fi

if [ -n "$VPN_CONTAINER" ]; then
    if ! is_running_container "$VPN_CONTAINER"; then
        if [ "$VPN_CONTAINER_EXPLICIT" = "1" ]; then
            die "AMNEZIA_UPDATE_VPN_CONTAINER must name a running VPN container"
        fi
        VPN_CONTAINER=""
    fi
fi

sudo docker rm -f "$CONTAINER_NAME" >/dev/null 2>&1 || true
sudo docker rm -f "$HOST_CONTAINER_NAME" >/dev/null 2>&1 || true
for stale_container in $(sudo docker ps -a --format '{{.Names}}' | grep "^${CONTAINER_NAME}-vpn-" || true); do
    sudo docker rm -f "$stale_container" >/dev/null 2>&1 || true
done
PORT_ARGS=""

open_host_firewall_port

# shellcheck disable=SC2086
sudo docker run -d \
    --log-driver none \
    --restart always \
    --network "$NETWORK_NAME" \
    --ip "$BRIDGE_HOST" \
    --name "$CONTAINER_NAME" \
    $PORT_ARGS \
    -v "$HOST_DIRECTORY:/www:ro" \
    --entrypoint sh \
    "$IMAGE" \
    -c "busybox httpd -f -p $SYNC_PORT -h /www"

if [ -n "$VPN_CONTAINER" ]; then
    TUNNEL_CONTAINER="${CONTAINER_NAME}-vpn-${VPN_CONTAINER}"
    sudo docker rm -f "$TUNNEL_CONTAINER" >/dev/null 2>&1 || true
    sudo docker run -d \
        --log-driver none \
        --restart always \
        --network "container:$VPN_CONTAINER" \
        --name "$TUNNEL_CONTAINER" \
        -v "$HOST_DIRECTORY:/www:ro" \
        --entrypoint sh \
        "$IMAGE" \
        -c "busybox httpd -f -p $SYNC_PORT -h /www"

    sudo docker ps --format '{{.Names}}' | grep -qx "$TUNNEL_CONTAINER"
    wait_http_ready "$TUNNEL_CONTAINER" || die "Tunnel update endpoint did not become ready"
fi

sudo docker ps --format '{{.Names}}' | grep -qx "$CONTAINER_NAME"
wait_http_ready "$CONTAINER_NAME" || die "Bridge update endpoint did not become ready"

if [ "$PUBLISH_HOST_PORT" = "1" ]; then
    sudo docker run -d \
        --log-driver none \
        --restart always \
        --network host \
        --name "$HOST_CONTAINER_NAME" \
        -v "$HOST_DIRECTORY:/www:ro" \
        --entrypoint sh \
        "$IMAGE" \
        -c "busybox httpd -f -p ${HOST_BIND}:${SYNC_PORT} -h /www"

    sudo docker ps --format '{{.Names}}' | grep -qx "$HOST_CONTAINER_NAME"
fi
wait_host_http_ready || die "Host update endpoint did not become ready"
