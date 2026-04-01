package handlers

import (
	"fmt"
	"log"
	"strings"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

const (
	vipDNSNetworkName       = "amnezia-dns-net"
	vipDNSBootstrapVersion  = "20260331-1"
	vipDNSImageName         = "fblink-dns-runtime"
	vipDNSBootstrapDir      = "/opt/amnezia/dns-bootstrap"
	vipDNSConfigDir         = "/opt/amnezia/dns"
	vipDNSCleanContainer    = "fblink-dns"
	vipDNSAdBlockContainer  = "fblink-dns-adblock"
	vipDNSCleanIP           = "172.29.172.254"
	vipDNSAdBlockIP         = "172.29.172.253"
	vipDNSPiHoleIP          = "172.29.172.252" // отдельный IP, не конфликтует с fblink-dns-adblock
	vipDNSPublicPrimary     = "1.1.1.1"
	vipDNSPublicSecondary   = "8.8.8.8"
	vipDNSBlocklistURL      = "https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts"
	vipDNSFallbackBlocklist = `0.0.0.0 doubleclick.net
0.0.0.0 googleadservices.com
0.0.0.0 googlesyndication.com
0.0.0.0 adservice.google.com
0.0.0.0 pagead2.googlesyndication.com
0.0.0.0 amazon-adsystem.com
0.0.0.0 app-measurement.com
0.0.0.0 facebook-analytics.com
0.0.0.0 ads-twitter.com
0.0.0.0 analytics.tiktok.com
0.0.0.0 ads.yahoo.com
0.0.0.0 adnxs.com
0.0.0.0 adsrvr.org
0.0.0.0 scorecardresearch.com
0.0.0.0 hotjar.com
0.0.0.0 sentry.io`
)

type vipDNSConfig struct {
	Primary       string
	Secondary     string
	Requested     bool
	Applied       bool
	Status        string
	Source        string
	DegradeReason string
}

const (
	vipAdBlockStatusApplied     = "applied"
	vipAdBlockStatusDegraded    = "degraded"
	vipAdBlockStatusUnavailable = "unavailable"

	vipAdBlockDegradeReasonNone        = ""
	vipAdBlockDegradeReasonAuthExpired = "auth_expired"
	vipAdBlockDegradeReasonSyncStale   = "sync_stale"
	vipAdBlockDegradeReasonDNSDown     = "dns_unreachable"
	vipAdBlockDegradeReasonRulesMissed = "routing_rules_missing"
)

func vipDNSCleanConfig(requested bool, status, degradeReason string) vipDNSConfig {
	return vipDNSConfig{
		Primary:       vipDNSPublicPrimary,
		Secondary:     vipDNSPublicSecondary,
		Requested:     requested,
		Applied:       false,
		Status:        status,
		Source:        piHoleDNSSourceClean,
		DegradeReason: strings.TrimSpace(degradeReason),
	}
}

func vipDNSPiHoleConfig(primary, secondary, source string) vipDNSConfig {
	if secondary == "" {
		secondary = vipDNSPublicSecondary
	}
	return vipDNSConfig{
		Primary:       primary,
		Secondary:     secondary,
		Requested:     true,
		Applied:       true,
		Status:        vipAdBlockStatusApplied,
		Source:        source,
		DegradeReason: vipAdBlockDegradeReasonNone,
	}
}

func mapPiHoleDegradeReason(errorCode string) string {
	switch strings.TrimSpace(strings.ToLower(errorCode)) {
	case "auth_failed", "auth_error", "auth_expired":
		return vipAdBlockDegradeReasonAuthExpired
	case "dns_unreachable", "xray_unreachable", "xray_not_reachable":
		return vipAdBlockDegradeReasonDNSDown
	case "routing_rules_missing":
		return vipAdBlockDegradeReasonRulesMissed
	default:
		return vipAdBlockDegradeReasonSyncStale
	}
}

const vipDNSDockerfile = `FROM alpine:3.19

RUN apk add --no-cache bash curl ca-certificates dnsmasq bind-tools

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh && mkdir -p /opt/fblink-dns

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
`

const vipDNSEntrypoint = `#!/bin/sh
set -eu

mkdir -p /opt/fblink-dns /run/dnsmasq

if [ "${ADBLOCK_ENABLED:-0}" = "1" ]; then
  BLOCKLIST_URL="${BLOCKLIST_URL:-https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts}"
  TMP_FILE="/opt/fblink-dns/blocked.hosts.tmp"
  if curl -fsSL "$BLOCKLIST_URL" -o "$TMP_FILE" && grep -qE '(^0\.0\.0\.0|^127\.0\.0\.1)' "$TMP_FILE"; then
    mv "$TMP_FILE" /opt/fblink-dns/blocked.hosts
  else
    rm -f "$TMP_FILE"
  fi

  if [ ! -s /opt/fblink-dns/blocked.hosts ]; then
    cat > /opt/fblink-dns/blocked.hosts <<'EOF'
__FALLBACK_BLOCKLIST__
EOF
  fi
fi

set -- \
  --keep-in-foreground \
  --log-facility=- \
  --cache-size=10000 \
  --no-resolv \
  --server="${UPSTREAM_DNS_1:-1.1.1.1}" \
  --server="${UPSTREAM_DNS_2:-8.8.8.8}" \
  --listen-address=0.0.0.0 \
  --bind-interfaces

if [ "${ADBLOCK_ENABLED:-0}" = "1" ]; then
  set -- "$@" --addn-hosts=/opt/fblink-dns/blocked.hosts
fi

exec dnsmasq "$@"
`

func vipDNSResolvedEntrypoint() string {
	return strings.ReplaceAll(vipDNSEntrypoint, "__FALLBACK_BLOCKLIST__", vipDNSFallbackBlocklist)
}

func buildVIPDNSBootstrapCommand(targetXrayContainer string) string {
	return fmt.Sprintf(`set -eu
BOOTSTRAP_DIR=%s
CONFIG_DIR=%s
IMAGE_NAME=%s
NETWORK_NAME=%s
BOOTSTRAP_VERSION=%s
TARGET_XRAY_CONTAINER=%s
FORCE_RECREATE=0

mkdir -p "$BOOTSTRAP_DIR" "$CONFIG_DIR"

cat > "$BOOTSTRAP_DIR/Dockerfile" <<'FBLINK_DNS_DOCKERFILE'
%s
FBLINK_DNS_DOCKERFILE

cat > "$BOOTSTRAP_DIR/entrypoint.sh" <<'FBLINK_DNS_ENTRYPOINT'
%s
FBLINK_DNS_ENTRYPOINT

chmod +x "$BOOTSTRAP_DIR/entrypoint.sh"

CURRENT_VERSION=""
if [ -f "$BOOTSTRAP_DIR/version" ]; then
  CURRENT_VERSION="$(tr -d '\r\n' < "$BOOTSTRAP_DIR/version")"
fi

if [ "$CURRENT_VERSION" != "$BOOTSTRAP_VERSION" ] || ! docker image inspect "$IMAGE_NAME" >/dev/null 2>&1; then
  docker build -t "$IMAGE_NAME" "$BOOTSTRAP_DIR" >/dev/null
  printf '%%s' "$BOOTSTRAP_VERSION" > "$BOOTSTRAP_DIR/version"
  FORCE_RECREATE=1
fi

if ! docker network ls --format '{{.Name}}' | grep -qx "$NETWORK_NAME"; then
  docker network create --driver bridge --subnet=172.29.172.0/24 --opt com.docker.network.bridge.name=amn0 "$NETWORK_NAME" >/dev/null
fi

ensure_dns_container() {
  NAME="$1"
  IP_ADDR="$2"
  ADBLOCK="$3"
  if [ "$FORCE_RECREATE" = "1" ]; then
    docker rm -f "$NAME" >/dev/null 2>&1 || true
  fi

  if ! docker ps -a --format '{{.Names}}' | grep -qx "$NAME"; then
    docker run -d \
      --log-driver none \
      --restart always \
      --network "$NETWORK_NAME" \
      --ip "$IP_ADDR" \
      --name "$NAME" \
      -e "ADBLOCK_ENABLED=$ADBLOCK" \
      -e BLOCKLIST_URL=%s \
      -e UPSTREAM_DNS_1=%s \
      -e UPSTREAM_DNS_2=%s \
      "$IMAGE_NAME" >/dev/null
  else
    docker start "$NAME" >/dev/null 2>&1 || true
  fi
}

ensure_dns_container %s %s 0
ensure_dns_container %s %s 1

for XRAY_CONTAINER in "$TARGET_XRAY_CONTAINER" %s %s; do
  if [ -n "$XRAY_CONTAINER" ] && docker ps -a --format '{{.Names}}' | grep -qx "$XRAY_CONTAINER"; then
    docker network connect "$NETWORK_NAME" "$XRAY_CONTAINER" >/dev/null 2>&1 || true
  fi
done

wait_dns() {
  NAME="$1"
  for _ in 1 2 3 4 5 6 7 8 9 10; do
    if docker exec "$NAME" sh -lc "nslookup example.com 127.0.0.1 >/dev/null 2>&1"; then
      return 0
    fi
    sleep 1
  done
  docker logs "$NAME" --tail 40 2>/dev/null || true
  return 1
}

wait_dns %s
wait_dns %s
`, shellQuote(vipDNSBootstrapDir), shellQuote(vipDNSConfigDir), shellQuote(vipDNSImageName),
		shellQuote(vipDNSNetworkName), shellQuote(vipDNSBootstrapVersion), shellQuote(strings.TrimSpace(targetXrayContainer)),
		vipDNSDockerfile, vipDNSResolvedEntrypoint(), shellQuote(vipDNSBlocklistURL), shellQuote(vipDNSPublicPrimary),
		shellQuote(vipDNSPublicSecondary), shellQuote(vipDNSCleanContainer), shellQuote(vipDNSCleanIP),
		shellQuote(vipDNSAdBlockContainer), shellQuote(vipDNSAdBlockIP), shellQuote(defaultXrayContainer),
		shellQuote(legacyXrayContainer), shellQuote(vipDNSCleanContainer), shellQuote(vipDNSAdBlockContainer))
}

func ensureVIPDNSRuntime(server *models.VPNServer, targetXrayContainer string) error {
	if server == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return fmt.Errorf("ssh access is required for VIP DNS bootstrap")
	}

	_, err := sshExec(server, buildVIPDNSBootstrapCommand(targetXrayContainer))
	return err
}

func buildVIPPiHoleBootstrapCommand(targetXrayContainer string) string {
	return fmt.Sprintf(`set -eu
NETWORK_NAME=%s
PIHOLE_IP=%s
TARGET_XRAY_CONTAINER=%s

find_pihole_container() {
  docker ps --format '{{.Names}} {{.Image}}' | awk '
    BEGIN { IGNORECASE=1 }
    $1 == "pihole" || $1 == "pi-hole" || $2 ~ /pihole\/pihole/ { print $1; exit }
  '
}

PIHOLE_CONTAINER="$(find_pihole_container)"
if [ -z "$PIHOLE_CONTAINER" ]; then
  PIHOLE_CONTAINER="$(docker ps -a --format '{{.Names}} {{.Image}}' | awk '
    BEGIN { IGNORECASE=1 }
    $1 == "pihole" || $1 == "pi-hole" || $2 ~ /pihole\/pihole/ { print $1; exit }
  ')"
fi

if [ -z "$PIHOLE_CONTAINER" ]; then
  echo "pihole container not found" >&2
  exit 1
fi

if ! docker network ls --format '{{.Name}}' | grep -qx "$NETWORK_NAME"; then
  docker network create --driver bridge --subnet=172.29.172.0/24 --opt com.docker.network.bridge.name=amn0 "$NETWORK_NAME" >/dev/null
fi

for LEGACY_CONTAINER in %s %s; do
  if docker ps -a --format '{{.Names}}' | grep -qx "$LEGACY_CONTAINER"; then
    docker rm -f "$LEGACY_CONTAINER" >/dev/null 2>&1 || true
  fi
done

ensure_network_ip() {
  NAME="$1"
  IP_ADDR="$2"
  CURRENT_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'"$NETWORK_NAME"'"}}{{.IPAddress}}{{end}}' "$NAME" 2>/dev/null || true)"
  if [ "$CURRENT_IP" = "$IP_ADDR" ]; then
    return 0
  fi
  if [ -n "$CURRENT_IP" ]; then
    docker network disconnect "$NETWORK_NAME" "$NAME" >/dev/null 2>&1 || true
  fi
  docker network connect --ip "$IP_ADDR" "$NETWORK_NAME" "$NAME" >/dev/null
}

ensure_network_connected() {
  NAME="$1"
  if [ -z "$NAME" ]; then
    return 0
  fi
  if ! docker ps -a --format '{{.Names}}' | grep -qx "$NAME"; then
    return 0
  fi
  CURRENT_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'"$NETWORK_NAME"'"}}{{.IPAddress}}{{end}}' "$NAME" 2>/dev/null || true)"
  if [ -z "$CURRENT_IP" ]; then
    docker network connect "$NETWORK_NAME" "$NAME" >/dev/null 2>&1 || true
  fi
}

ensure_network_ip "$PIHOLE_CONTAINER" "$PIHOLE_IP"
ensure_network_connected "$TARGET_XRAY_CONTAINER"
ensure_network_connected %s
ensure_network_connected %s

PIHOLE_INTERNAL_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'"$NETWORK_NAME"'"}}{{.IPAddress}}{{end}}' "$PIHOLE_CONTAINER" 2>/dev/null || true)"
if [ "$PIHOLE_INTERNAL_IP" != "$PIHOLE_IP" ]; then
  echo "pihole internal ip mismatch: expected $PIHOLE_IP got ${PIHOLE_INTERNAL_IP:-<empty>}" >&2
  exit 1
fi
`, shellQuote(vipDNSNetworkName), shellQuote(vipDNSPiHoleIP), shellQuote(strings.TrimSpace(targetXrayContainer)),
		shellQuote(vipDNSCleanContainer), shellQuote(vipDNSAdBlockContainer), shellQuote(defaultXrayContainer), shellQuote(legacyXrayContainer))
}

func ensureVIPPiHoleRuntime(server *models.VPNServer, targetXrayContainer string) error {
	if server == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return fmt.Errorf("ssh access is required for VIP Pi-hole bootstrap")
	}

	_, err := sshExec(server, buildVIPPiHoleBootstrapCommand(targetXrayContainer))
	return err
}

func resolveVIPDNSConfig(db *gorm.DB, server *models.VPNServer, template *models.VLESSServerTemplate, sub models.Subscription) vipDNSConfig {
	if !sub.VIPAdBlockEnabled {
		return vipDNSCleanConfig(false, vipAdBlockStatusUnavailable, vipAdBlockDegradeReasonNone)
	}

	if server == nil || template == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return vipDNSCleanConfig(true, vipAdBlockStatusDegraded, vipAdBlockDegradeReasonSyncStale)
	}

	if !server.PiHoleEnabled || server.PiHoleMode == piholeDisabled {
		log.Printf("[VIP DNS] Pi-hole disabled on %s while ad block requested", server.Name)
		return vipDNSCleanConfig(true, vipAdBlockStatusDegraded, vipAdBlockDegradeReasonSyncStale)
	}

	if cachedIP := strings.TrimSpace(server.PiHoleDNSIP); cachedIP != "" && strings.TrimSpace(server.PiHoleLastSyncError) == "" {
		source := piHoleDNSSourceHost
		if strings.TrimSpace(server.PiHoleLastMode) == piholeDocker {
			source = piHoleDNSSourceDocker
		}
		log.Printf("[VIP DNS] using healthy cached Pi-hole on %s: dns=%s source=%s", server.Name, cachedIP, source)
		return vipDNSPiHoleConfig(cachedIP, vipDNSPublicSecondary, source)
	}

	targetXrayContainer := resolvePiHoleTargetXrayFn(server, template.ContainerName)
	result := syncPiHoleServerFn(server, targetXrayContainer)
	if persistErr := persistPiHoleSyncResult(db, server, result); persistErr != nil {
		log.Printf("[VIP DNS] failed to persist Pi-hole sync result for %s: %v", server.Name, persistErr)
	}

	if result.Error == "" && strings.TrimSpace(result.DNSIP) != "" {
		log.Printf("[VIP DNS] self-healed Pi-hole on %s: dns=%s source=%s", server.Name, result.DNSIP, result.DNSSource)
		return vipDNSPiHoleConfig(result.DNSIP, vipDNSPublicSecondary, result.DNSSource)
	}

	log.Printf("[VIP DNS] degraded on %s: requested adblock but Pi-hole unavailable (%s: %s)", server.Name, result.ErrorCode, result.Error)
	return vipDNSCleanConfig(true, vipAdBlockStatusDegraded, mapPiHoleDegradeReason(result.ErrorCode))
}
