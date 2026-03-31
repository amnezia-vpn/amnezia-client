package handlers

import (
	"fmt"
	"log"
	"sort"
	"strings"
	"time"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

// Канонические Pi-hole adlists для VIP группы (v1)
const (
	piHoleCanonicalListStevenBlack = "https://raw.githubusercontent.com/StevenBlack/hosts/master/hosts"
	piHoleCanonicalListOISD        = "https://big.oisd.nl"
	piHoleDefaultGroup             = "VIP"
	piHoleDockerDefaultContainer   = "pihole"
	piHoleDockerImage              = "pihole/pihole:latest"
	piHoleDockerConfigDir          = "/opt/pihole"
)

const (
	piHoleDNSSourceHost   = "pihole_host"
	piHoleDNSSourceDocker = "pihole_docker"
	piHoleDNSSourceClean  = "clean"
)

const (
	piHoleErrDisabled          = "disabled"
	piHoleErrSSHRequired       = "ssh_required"
	piHoleErrNotFound          = "pihole_not_found"
	piHoleErrDeployFailed      = "deploy_failed"
	piHoleErrRuntimeNotFound   = "xray_runtime_not_found"
	piHoleErrReachability      = "xray_reachability_failed"
	piHoleErrDBNotFound        = "db_not_found"
	piHoleErrSQLiteUnavailable = "sqlite_unavailable"
	piHoleErrSchemaMismatch    = "schema_mismatch"
	piHoleErrRecoverFailed     = "gravity_recover_failed"
	piHoleErrSeedFailed        = "gravity_seed_failed"
)

// piHoleSyncResult — результат синхронизации Pi-hole на одном сервере
type piHoleSyncResult struct {
	ServerID      uint   `json:"server_id"`
	ServerName    string `json:"server_name"`
	ModeDetected  string `json:"mode_detected"` // host | docker | none
	DNSIP         string `json:"dns_ip"`
	GroupSynced   bool   `json:"group_synced"`
	ListsSynced   int    `json:"lists_synced"`
	XrayReachable bool   `json:"xray_reachable"`
	DBPath        string `json:"db_path,omitempty"`
	ClientIP      string `json:"client_ip,omitempty"`
	ClientSynced  bool   `json:"client_synced"`
	DNSSource     string `json:"dns_source,omitempty"`
	ErrorCode     string `json:"error_code,omitempty"`
	Error         string `json:"error,omitempty"`
}

type piHoleDBInspection struct {
	Path   string
	Tables []string
}

type piHoleReachability struct {
	DNSIP         string
	ClientIP      string
	DNSSource     string
	XrayReachable bool
}

var (
	syncPiHoleServerFn        = syncPiHoleServer
	resolvePiHoleTargetXrayFn = resolvePiHoleTargetXrayContainer
)

// piHoleGroupName возвращает имя Pi-hole группы для сервера (дефолт VIP)
func piHoleGroupName(server *models.VPNServer) string {
	if server.PiHoleGroupName != "" {
		return server.PiHoleGroupName
	}
	return piHoleDefaultGroup
}

// piHoleContainerName возвращает имя Docker-контейнера Pi-hole для сервера
func piHoleContainerName(server *models.VPNServer) string {
	if server.PiHoleContainerName != "" {
		return server.PiHoleContainerName
	}
	return piHoleDockerDefaultContainer
}

func canonicalPiHoleLists() []string {
	return []string{piHoleCanonicalListStevenBlack, piHoleCanonicalListOISD}
}

func parseKeyValueOutput(out string) map[string]string {
	parsed := make(map[string]string)
	for _, line := range strings.Split(out, "\n") {
		line = strings.TrimSpace(line)
		if line == "" {
			continue
		}
		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}
		parsed[strings.TrimSpace(parts[0])] = strings.TrimSpace(parts[1])
	}
	return parsed
}

func parseCommaSeparatedList(raw string) []string {
	raw = strings.TrimSpace(raw)
	if raw == "" {
		return nil
	}
	items := strings.Split(raw, ",")
	result := make([]string, 0, len(items))
	for _, item := range items {
		item = strings.TrimSpace(item)
		if item == "" {
			continue
		}
		result = append(result, item)
	}
	sort.Strings(result)
	return result
}

func missingPiHoleTables(tables []string) []string {
	required := []string{"group", "adlist", "adlist_by_group", "client", "client_by_group"}
	have := make(map[string]struct{}, len(tables))
	for _, table := range tables {
		have[table] = struct{}{}
	}

	missing := make([]string, 0)
	for _, table := range required {
		if _, ok := have[table]; !ok {
			missing = append(missing, table)
		}
	}
	return missing
}

func piHoleExecCommand(mode, containerName, script string) string {
	if mode == piholeDocker {
		return fmt.Sprintf("docker exec %s sh -lc %s", shellQuote(containerName), shellQuote(script))
	}
	return fmt.Sprintf("sh -lc %s", shellQuote(script))
}

func buildDetectPiHoleCommand(preferredContainerName string) string {
	return fmt.Sprintf(`set -eu

PIHOLE_CONTAINER=""
for NAME in %s pihole pi-hole; do
  if [ -n "$NAME" ] && docker ps --format '{{.Names}}' 2>/dev/null | grep -qx "$NAME"; then
    PIHOLE_CONTAINER="$NAME"
    break
  fi
done

if [ -z "$PIHOLE_CONTAINER" ]; then
  PIHOLE_CONTAINER="$(docker ps --format '{{.Names}} {{.Image}}' 2>/dev/null | awk '$2 ~ /pihole\/pihole/ { print $1; exit }' || true)"
fi

if [ -n "$PIHOLE_CONTAINER" ]; then
  printf 'docker:%%s\n' "$PIHOLE_CONTAINER"
  exit 0
fi

if command -v pihole >/dev/null 2>&1; then
  printf 'host\n'
  exit 0
fi

if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet pihole-FTL 2>/dev/null; then
  printf 'host\n'
  exit 0
fi

printf ''\n`, shellQuote(preferredContainerName))
}

func detectPiHole(server *models.VPNServer) (mode string, containerName string, err error) {
	if server == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return "", "", fmt.Errorf("SSH access required for Pi-hole detection")
	}

	if server.PiHoleMode == piholeDisabled {
		return "", "", nil
	}

	out, execErr := sshExec(server, buildDetectPiHoleCommand(piHoleContainerName(server)))
	if execErr != nil {
		return "", "", fmt.Errorf("Pi-hole detection SSH failed: %w", execErr)
	}

	out = strings.TrimSpace(out)
	if out == "" {
		return "", "", nil
	}
	if strings.HasPrefix(out, "docker:") {
		return piholeDocker, strings.TrimPrefix(out, "docker:"), nil
	}
	if out == piholeHost {
		return piholeHost, "", nil
	}
	return "", "", nil
}

const (
	piholeAuto     = "auto"
	piholeHost     = "host"
	piholeDocker   = "docker"
	piholeDisabled = "disabled"
)

func buildInspectPiHoleDBScript() string {
	return `set -eu

ensure_sqlite3() {
  if command -v sqlite3 >/dev/null 2>&1; then
    return 0
  fi
  if command -v apt-get >/dev/null 2>&1; then
    apt-get update -qq >/dev/null 2>&1 || true
    apt-get install -y sqlite3 -qq >/dev/null 2>&1 || true
  elif command -v apk >/dev/null 2>&1; then
    apk add --no-cache sqlite >/dev/null 2>&1 || true
  elif command -v dnf >/dev/null 2>&1; then
    dnf install -y sqlite >/dev/null 2>&1 || true
  elif command -v yum >/dev/null 2>&1; then
    yum install -y sqlite >/dev/null 2>&1 || true
  fi
  command -v sqlite3 >/dev/null 2>&1
}

sql_engine_ready() {
  if command -v sqlite3 >/dev/null 2>&1; then
    return 0
  fi
  if command -v python3 >/dev/null 2>&1; then
    return 0
  fi
  ensure_sqlite3 >/dev/null 2>&1
}

list_db_candidates() {
  maybe_emit() {
    KIND="$1"
    CANDIDATE="$2"
    if [ -n "$CANDIDATE" ] && [ -f "$CANDIDATE" ]; then
      printf '%s|%s\n' "$KIND" "$CANDIDATE"
    fi
  }

  maybe_emit gravity "$(pihole-FTL --config files.gravity 2>/dev/null || true)"
  maybe_emit ftl "$(pihole-FTL --config files.database 2>/dev/null || true)"
  maybe_emit gravity /etc/pihole/gravity.db
  maybe_emit gravity /opt/pihole/gravity.db
  maybe_emit ftl /etc/pihole/pihole-FTL.db
  maybe_emit ftl /opt/pihole/pihole-FTL.db

  find /etc/pihole /opt/pihole -maxdepth 3 \( -name 'gravity.db' -o -name 'pihole-FTL.db' \) 2>/dev/null | while read -r CANDIDATE; do
    case "$CANDIDATE" in
      *gravity.db) printf 'gravity|%s\n' "$CANDIDATE" ;;
      *pihole-FTL.db) printf 'ftl|%s\n' "$CANDIDATE" ;;
    esac
  done
}

run_sql() {
  DB_FILE="$1"
  QUERY="$2"
  if command -v sqlite3 >/dev/null 2>&1 || ensure_sqlite3; then
    printf '%s\n' "$QUERY" | sqlite3 "$DB_FILE"
    return 0
  fi
  if command -v python3 >/dev/null 2>&1; then
    DB_PATH="$DB_FILE" SQL_QUERY="$QUERY" python3 - <<'PY'
import os
import sqlite3
import sys

db_path = os.environ["DB_PATH"]
query = os.environ["SQL_QUERY"]

conn = sqlite3.connect(db_path)
try:
    cur = conn.execute(query)
    rows = cur.fetchall()
    for row in rows:
        if len(row) == 1:
            print("" if row[0] is None else row[0])
        else:
            print("|".join("" if item is None else str(item) for item in row))
finally:
    conn.close()
PY
    return 0
  fi
  return 1
}

if ! sql_engine_ready; then
  FALLBACK_DB="$(list_db_candidates | awk -F'|' '!seen[$2]++ && NF >= 2 {print $2; exit}')"
  if [ -n "$FALLBACK_DB" ]; then
    echo "DB_PATH=$FALLBACK_DB"
  fi
  echo "ERROR_CODE=sqlite_unavailable"
  exit 12
fi

BEST_GRAVITY_DB=""
BEST_GRAVITY_TABLES=""
BEST_GRAVITY_SCORE=-1

BEST_FALLBACK_DB=""
BEST_FALLBACK_TABLES=""
BEST_FALLBACK_SCORE=-1

for ENTRY in $(list_db_candidates | awk -F'|' '!seen[$2]++ && NF >= 2 {print $1 "|" $2}'); do
  KIND="${ENTRY%%|*}"
  DB_PATH="${ENTRY#*|}"
  [ -f "$DB_PATH" ] || continue
  TABLES="$(run_sql "$DB_PATH" "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' ORDER BY name;" | paste -sd, - || true)"

  SCORE=0
  for REQUIRED in group adlist adlist_by_group client client_by_group; do
    case ",$TABLES," in
      *",$REQUIRED,"*) SCORE=$((SCORE + 1)) ;;
    esac
  done

  if [ "$KIND" = "gravity" ]; then
    if [ "$SCORE" -gt "$BEST_GRAVITY_SCORE" ]; then
      BEST_GRAVITY_SCORE="$SCORE"
      BEST_GRAVITY_DB="$DB_PATH"
      BEST_GRAVITY_TABLES="$TABLES"
    fi
    continue
  fi

  if [ "$SCORE" -gt "$BEST_FALLBACK_SCORE" ]; then
    BEST_FALLBACK_SCORE="$SCORE"
    BEST_FALLBACK_DB="$DB_PATH"
    BEST_FALLBACK_TABLES="$TABLES"
  fi
done

BEST_DB="$BEST_GRAVITY_DB"
BEST_TABLES="$BEST_GRAVITY_TABLES"
if [ -z "$BEST_DB" ]; then
  BEST_DB="$BEST_FALLBACK_DB"
  BEST_TABLES="$BEST_FALLBACK_TABLES"
fi

if [ -z "$BEST_DB" ]; then
  FALLBACK_DB="$(list_db_candidates | awk -F'|' '!seen[$2]++ && NF >= 2 {print $2; exit}')"
  if [ -n "$FALLBACK_DB" ]; then
    echo "DB_PATH=$FALLBACK_DB"
  fi
  echo "ERROR_CODE=sqlite_unavailable"
  exit 12
fi

echo "DB_PATH=$BEST_DB"
echo "TABLES=$BEST_TABLES"`
}

func inspectPiHoleDB(server *models.VPNServer, mode, containerName string) (piHoleDBInspection, string, error) {
	out, err := sshExec(server, piHoleExecCommand(mode, containerName, buildInspectPiHoleDBScript()))
	parsed := parseKeyValueOutput(out)
	info := piHoleDBInspection{
		Path:   strings.TrimSpace(parsed["DB_PATH"]),
		Tables: parseCommaSeparatedList(parsed["TABLES"]),
	}
	errorCode := parsed["ERROR_CODE"]
	if err != nil {
		if errorCode == "" {
			errorCode = piHoleErrDBNotFound
		}
		return info, errorCode, fmt.Errorf("Pi-hole DB inspect failed: %w (output: %s)", err, strings.TrimSpace(out))
	}
	if info.Path == "" {
		if errorCode == "" {
			errorCode = piHoleErrDBNotFound
		}
		return info, errorCode, fmt.Errorf("Pi-hole DB path not resolved")
	}
	return info, "", nil
}

func buildRecoverPiHoleGravityCommand(mode, containerName string) string {
	script := `set -eu

if ! command -v pihole >/dev/null 2>&1; then
  echo "ERROR_CODE=gravity_recover_failed"
  exit 51
fi

if pihole -g -r recover >/dev/null 2>&1; then
  echo "RECOVERED=1"
  exit 0
fi

if pihole updateGravity >/dev/null 2>&1 || pihole -g >/dev/null 2>&1; then
  echo "RECOVERED=1"
  exit 0
fi

echo "ERROR_CODE=gravity_recover_failed"
exit 51`

	return piHoleExecCommand(mode, containerName, script)
}

func attemptRecoverPiHoleGravityDB(server *models.VPNServer, mode, containerName string) error {
	out, err := sshExec(server, buildRecoverPiHoleGravityCommand(mode, containerName))
	if err != nil {
		return fmt.Errorf("Pi-hole gravity recovery failed: %w (output: %s)", err, strings.TrimSpace(out))
	}
	if !strings.Contains(out, "RECOVERED=1") {
		return fmt.Errorf("Pi-hole gravity recovery did not confirm success (output: %s)", strings.TrimSpace(out))
	}
	return nil
}

func buildPiHoleSeedSQL(groupName, clientIP string) string {
	sql := fmt.Sprintf(`
INSERT OR IGNORE INTO "group" (enabled, name, description)
  VALUES (1, %s, 'FBLink VIP AdBlock');
`, sqliteQuote(groupName))

	for _, url := range canonicalPiHoleLists() {
		sql += fmt.Sprintf(`
INSERT OR IGNORE INTO adlist (address, enabled, comment)
  VALUES (%s, 1, 'FBLink VIP canonical adblock');
`, sqliteQuote(url))
	}

	listValues := make([]string, 0, len(canonicalPiHoleLists()))
	for _, url := range canonicalPiHoleLists() {
		listValues = append(listValues, sqliteQuote(url))
	}

	sql += fmt.Sprintf(`
INSERT OR IGNORE INTO adlist_by_group (adlist_id, group_id)
  SELECT a.id, g.id
  FROM adlist a
  JOIN "group" g ON g.name = %s
  WHERE a.address IN (%s);
`, sqliteQuote(groupName), strings.Join(listValues, ", "))

	if clientIP != "" {
		sql += fmt.Sprintf(`
INSERT OR IGNORE INTO client (ip, comment)
  VALUES (%s, 'FBLink VIP XRay');

INSERT OR IGNORE INTO client_by_group (client_id, group_id)
  SELECT c.id, g.id
  FROM client c
  JOIN "group" g ON g.name = %s
  WHERE c.ip = %s;
`, sqliteQuote(clientIP), sqliteQuote(groupName), sqliteQuote(clientIP))
	}

	return sql
}

func sqliteQuote(s string) string {
	return "'" + strings.ReplaceAll(s, "'", "''") + "'"
}

func buildEnsurePiHoleBaseConfigCommand(mode, containerName, dbPath, groupName, clientIP string) string {
	sql := buildPiHoleSeedSQL(groupName, clientIP)
	listValues := make([]string, 0, len(canonicalPiHoleLists()))
	for _, url := range canonicalPiHoleLists() {
		listValues = append(listValues, sqliteQuote(url))
	}
	listSQL := strings.Join(listValues, ", ")
	clientBeforeSQL := "0"
	clientAfterSQL := "0"
	if clientIP != "" {
		clientBeforeSQL = fmt.Sprintf("SELECT COUNT(*) FROM client WHERE ip = %s;", sqliteQuote(clientIP))
		clientAfterSQL = clientBeforeSQL
	}

	script := fmt.Sprintf(`set -eu
DB_PATH=%s
CLIENT_IP=%s

ensure_sqlite3() {
  if command -v sqlite3 >/dev/null 2>&1; then
    return 0
  fi
  if command -v apt-get >/dev/null 2>&1; then
    apt-get update -qq >/dev/null 2>&1 || true
    apt-get install -y sqlite3 -qq >/dev/null 2>&1 || true
  elif command -v apk >/dev/null 2>&1; then
    apk add --no-cache sqlite >/dev/null 2>&1 || true
  elif command -v dnf >/dev/null 2>&1; then
    dnf install -y sqlite >/dev/null 2>&1 || true
  elif command -v yum >/dev/null 2>&1; then
    yum install -y sqlite >/dev/null 2>&1 || true
  fi
  command -v sqlite3 >/dev/null 2>&1
}

run_query() {
  QUERY="$1"
  if command -v sqlite3 >/dev/null 2>&1 || ensure_sqlite3; then
    printf '%%s\n' "$QUERY" | sqlite3 "$DB_PATH"
    return 0
  fi
  if command -v python3 >/dev/null 2>&1; then
    DB_PATH="$DB_PATH" SQL_QUERY="$QUERY" python3 - <<'PY'
import os
import sqlite3
import sys

db_path = os.environ["DB_PATH"]
query = os.environ["SQL_QUERY"]

conn = sqlite3.connect(db_path)
try:
    cur = conn.execute(query)
    rows = cur.fetchall()
    for row in rows:
        if len(row) == 1:
            print("" if row[0] is None else row[0])
        else:
            print("|".join("" if item is None else str(item) for item in row))
finally:
    conn.close()
PY
    return 0
  fi
  return 1
}

run_script() {
  if command -v sqlite3 >/dev/null 2>&1 || ensure_sqlite3; then
    sqlite3 "$DB_PATH"
    return 0
  fi
  if command -v python3 >/dev/null 2>&1; then
    DB_PATH="$DB_PATH" python3 - <<'PY'
import os
import sqlite3
import sys

db_path = os.environ["DB_PATH"]
script = sys.stdin.read()

conn = sqlite3.connect(db_path)
try:
    conn.executescript(script)
    conn.commit()
finally:
    conn.close()
PY
    return 0
  fi
  return 1
}

LISTS_BEFORE="$(run_query "SELECT COUNT(*) FROM adlist WHERE address IN (%s);" | tr -d '\r\n' || true)"
CLIENT_BEFORE="$(run_query %s | tr -d '\r\n' || true)"

cat > /tmp/_pihole_seed.sql <<'PIHOLE_SQL'
%s
PIHOLE_SQL

if ! run_script < /tmp/_pihole_seed.sql; then
  rm -f /tmp/_pihole_seed.sql
  echo "ERROR_CODE=gravity_seed_failed"
  exit 21
fi
rm -f /tmp/_pihole_seed.sql

LISTS_AFTER="$(run_query "SELECT COUNT(*) FROM adlist WHERE address IN (%s);" | tr -d '\r\n' || true)"
CLIENT_AFTER="$(run_query %s | tr -d '\r\n' || true)"

REFRESH_MODE="reloadlists"
if [ "$LISTS_BEFORE" != "$LISTS_AFTER" ]; then
  pihole -g > /dev/null 2>&1 || pihole updateGravity > /dev/null 2>&1 || pihole gravity > /dev/null 2>&1 || true
  REFRESH_MODE="gravity"
else
  pihole reloadlists > /dev/null 2>&1 || pihole -g > /dev/null 2>&1 || pihole updateGravity > /dev/null 2>&1 || true
fi

echo "LISTS_SYNCED=$LISTS_AFTER"
echo "CLIENT_SYNCED=$CLIENT_AFTER"
echo "REFRESH_MODE=$REFRESH_MODE"
echo "SEED_OK=1"`,
		shellQuote(dbPath),
		shellQuote(clientIP),
		listSQL,
		shellQuote(clientBeforeSQL),
		sql,
		listSQL,
		shellQuote(clientAfterSQL),
	)

	return piHoleExecCommand(mode, containerName, script)
}

func ensurePiHoleBaseConfig(server *models.VPNServer, mode, containerName, dbPath, clientIP string) (int, bool, string, error) {
	if server == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return 0, false, "", fmt.Errorf("SSH access required for Pi-hole base config")
	}

	groupName := piHoleGroupName(server)
	out, err := sshExec(server, buildEnsurePiHoleBaseConfigCommand(mode, containerName, dbPath, groupName, clientIP))
	parsed := parseKeyValueOutput(out)
	if err != nil {
		errorCode := parsed["ERROR_CODE"]
		if errorCode == "" {
			errorCode = piHoleErrSeedFailed
		}
		return 0, false, errorCode, fmt.Errorf("Pi-hole gravity.db seeding failed: %w (output: %s)", err, strings.TrimSpace(out))
	}

	if parsed["SEED_OK"] != "1" {
		errorCode := parsed["ERROR_CODE"]
		if errorCode == "" {
			errorCode = piHoleErrSeedFailed
		}
		return 0, false, errorCode, fmt.Errorf("Pi-hole seeding did not confirm success (output: %s)", strings.TrimSpace(out))
	}

	clientSynced := false
	if parsed["CLIENT_SYNCED"] != "" && parsed["CLIENT_SYNCED"] != "0" {
		clientSynced = true
	}

	return len(canonicalPiHoleLists()), clientSynced, "", nil
}

func buildDeployDockerPiHoleCommand(containerName, networkName, piHoleIP string) string {
	return fmt.Sprintf(`set -eu

CONTAINER_NAME=%s
NETWORK_NAME=%s
PIHOLE_IP=%s
PIHOLE_IMAGE=%s
CONFIG_DIR=%s

if ! docker network ls --format '{{.Name}}' | grep -qx "$NETWORK_NAME"; then
  docker network create --driver bridge --subnet=172.29.172.0/24 \
    --opt com.docker.network.bridge.name=amn0 "$NETWORK_NAME" >/dev/null
fi

if docker ps -a --format '{{.Names}}' | grep -qx "$CONTAINER_NAME"; then
  docker start "$CONTAINER_NAME" >/dev/null 2>&1 || true
else
  mkdir -p "$CONFIG_DIR/etc-pihole" "$CONFIG_DIR/etc-dnsmasq.d"
  docker run -d \
    --name "$CONTAINER_NAME" \
    --restart always \
    --log-driver none \
    --cap-add NET_ADMIN \
    -e TZ="Europe/Moscow" \
    -e WEBPASSWORD="" \
    -e PIHOLE_DNS_="1.1.1.1;8.8.8.8" \
    -e DNSMASQ_LISTENING=all \
    -v "${CONFIG_DIR}/etc-pihole:/etc/pihole" \
    -v "${CONFIG_DIR}/etc-dnsmasq.d:/etc/dnsmasq.d" \
    "$PIHOLE_IMAGE" >/dev/null
fi

CURRENT_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'$NETWORK_NAME'"}}{{.IPAddress}}{{end}}' "$CONTAINER_NAME" 2>/dev/null || true)"
if [ "$CURRENT_IP" != "$PIHOLE_IP" ]; then
  docker network disconnect "$NETWORK_NAME" "$CONTAINER_NAME" >/dev/null 2>&1 || true
  docker network connect --ip "$PIHOLE_IP" "$NETWORK_NAME" "$CONTAINER_NAME" >/dev/null
fi

for _ in 1 2 3 4 5 6 7 8 9 10; do
  if docker exec "$CONTAINER_NAME" pihole status >/dev/null 2>&1; then
    break
  fi
  sleep 2
done

echo "PIHOLE_DEPLOYED=1"`,
		shellQuote(containerName),
		shellQuote(vipDNSNetworkName),
		shellQuote(piHoleIP),
		shellQuote(piHoleDockerImage),
		shellQuote(piHoleDockerConfigDir),
	)
}

func deployDockerPiHole(server *models.VPNServer) (string, error) {
	if server == nil || strings.TrimSpace(server.SSHPassword) == "" {
		return "", fmt.Errorf("SSH access required for Docker Pi-hole deploy")
	}

	containerName := piHoleContainerName(server)
	out, err := sshExec(server, buildDeployDockerPiHoleCommand(containerName, vipDNSNetworkName, vipDNSPiHoleIP))
	if err != nil {
		return "", fmt.Errorf("Docker Pi-hole deploy failed: %w (output: %s)", err, strings.TrimSpace(out))
	}
	if !strings.Contains(out, "PIHOLE_DEPLOYED=1") {
		return "", fmt.Errorf("Docker Pi-hole deploy did not confirm success (output: %s)", strings.TrimSpace(out))
	}
	return containerName, nil
}

func buildPiHoleDockerReachabilityCommand(piHoleContainer, targetXrayContainer string) string {
	return fmt.Sprintf(`set -eu

NETWORK_NAME=%s
PIHOLE_IP=%s
PIHOLE_CONTAINER=%s
TARGET_XRAY=%s

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
  if [ -z "$NAME" ] || ! docker ps -a --format '{{.Names}}' | grep -qx "$NAME"; then
    return 1
  fi
  CURRENT_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'"$NETWORK_NAME"'"}}{{.IPAddress}}{{end}}' "$NAME" 2>/dev/null || true)"
  if [ -z "$CURRENT_IP" ]; then
    docker network connect "$NETWORK_NAME" "$NAME" >/dev/null 2>&1 || true
  fi
  return 0
}

if ! docker network ls --format '{{.Name}}' | grep -qx "$NETWORK_NAME"; then
  docker network create --driver bridge --subnet=172.29.172.0/24 --opt com.docker.network.bridge.name=amn0 "$NETWORK_NAME" >/dev/null
fi

ensure_network_ip "$PIHOLE_CONTAINER" "$PIHOLE_IP"

if ! ensure_network_connected "$TARGET_XRAY"; then
  echo "ERROR_CODE=xray_runtime_not_found"
  exit 31
fi

XRAY_IP="$(docker inspect -f '{{with index .NetworkSettings.Networks "'"$NETWORK_NAME"'"}}{{.IPAddress}}{{end}}' "$TARGET_XRAY" 2>/dev/null || true)"
if [ -z "$XRAY_IP" ]; then
  echo "ERROR_CODE=xray_runtime_not_found"
  exit 32
fi

if ! docker exec "$TARGET_XRAY" sh -lc 'apk add --no-cache bind-tools >/dev/null 2>&1 || true; nslookup example.com '"$PIHOLE_IP"' >/dev/null 2>&1'; then
  echo "ERROR_CODE=xray_reachability_failed"
  exit 33
fi

echo "DNS_IP=$PIHOLE_IP"
echo "CLIENT_IP=$XRAY_IP"
echo "XrayReachable=1"`,
		shellQuote(vipDNSNetworkName),
		shellQuote(vipDNSPiHoleIP),
		shellQuote(piHoleContainer),
		shellQuote(strings.TrimSpace(targetXrayContainer)),
	)
}

func ensurePiHoleDockerReachability(server *models.VPNServer, piHoleContainer, targetXrayContainer string) (piHoleReachability, string, error) {
	out, err := sshExec(server, buildPiHoleDockerReachabilityCommand(piHoleContainer, targetXrayContainer))
	parsed := parseKeyValueOutput(out)
	reach := piHoleReachability{
		DNSIP:         strings.TrimSpace(parsed["DNS_IP"]),
		ClientIP:      strings.TrimSpace(parsed["CLIENT_IP"]),
		DNSSource:     piHoleDNSSourceDocker,
		XrayReachable: parsed["XrayReachable"] == "1",
	}
	errorCode := parsed["ERROR_CODE"]
	if err != nil {
		if errorCode == "" {
			errorCode = piHoleErrReachability
		}
		return reach, errorCode, fmt.Errorf("Pi-hole docker reachability failed: %w (output: %s)", err, strings.TrimSpace(out))
	}
	if reach.DNSIP == "" || reach.ClientIP == "" {
		if errorCode == "" {
			errorCode = piHoleErrReachability
		}
		return reach, errorCode, fmt.Errorf("Pi-hole docker reachability did not return dns/client IP")
	}
	return reach, "", nil
}

func buildVIPHostPiHoleGatewayCommand(targetXrayContainer string) string {
	return fmt.Sprintf(`set -eu
TARGET_XRAY_CONTAINER=%s

has_pihole_host() {
  if command -v pihole >/dev/null 2>&1; then
    return 0
  fi
  if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet pihole-FTL 2>/dev/null; then
    return 0
  fi
  return 1
}

if ! has_pihole_host; then
  echo "ERROR_CODE=pihole_not_found"
  exit 41
fi

if [ -z "$TARGET_XRAY_CONTAINER" ] || ! docker ps -a --format '{{.Names}}' | grep -qx "$TARGET_XRAY_CONTAINER"; then
  echo "ERROR_CODE=xray_runtime_not_found"
  exit 42
fi

NETWORKS="$(docker inspect -f '{{range $name, $cfg := .NetworkSettings.Networks}}{{if and $cfg.IPAddress $cfg.Gateway}}{{printf "%%s|%%s|%%s\n" $name $cfg.IPAddress $cfg.Gateway}}{{end}}{{end}}' "$TARGET_XRAY_CONTAINER" || true)"

DNS_IP=""
CLIENT_IP=""
while IFS='|' read -r NETNAME IPADDR GATEWAY; do
  [ -n "$IPADDR" ] || continue
  [ -n "$GATEWAY" ] || continue
  if docker exec "$TARGET_XRAY_CONTAINER" sh -lc 'apk add --no-cache bind-tools >/dev/null 2>&1 || true; nslookup example.com '"$GATEWAY"' >/dev/null 2>&1'; then
    DNS_IP="$GATEWAY"
    CLIENT_IP="$IPADDR"
    break
  fi
done <<NETWORK_EOF
$NETWORKS
NETWORK_EOF

if [ -z "$DNS_IP" ] || [ -z "$CLIENT_IP" ]; then
  echo "ERROR_CODE=xray_reachability_failed"
  exit 43
fi

echo "DNS_IP=$DNS_IP"
echo "CLIENT_IP=$CLIENT_IP"
echo "XrayReachable=1"`, shellQuote(strings.TrimSpace(targetXrayContainer)))
}

func resolveVIPHostPiHoleGateway(server *models.VPNServer, targetXrayContainer string) (piHoleReachability, string, error) {
	out, err := sshExec(server, buildVIPHostPiHoleGatewayCommand(targetXrayContainer))
	parsed := parseKeyValueOutput(out)
	reach := piHoleReachability{
		DNSIP:         strings.TrimSpace(parsed["DNS_IP"]),
		ClientIP:      strings.TrimSpace(parsed["CLIENT_IP"]),
		DNSSource:     piHoleDNSSourceHost,
		XrayReachable: parsed["XrayReachable"] == "1",
	}
	errorCode := parsed["ERROR_CODE"]
	if err != nil {
		if errorCode == "" {
			errorCode = piHoleErrReachability
		}
		return reach, errorCode, fmt.Errorf("host Pi-hole gateway resolve failed: %w (output: %s)", err, strings.TrimSpace(out))
	}
	if reach.DNSIP == "" || reach.ClientIP == "" {
		if errorCode == "" {
			errorCode = piHoleErrReachability
		}
		return reach, errorCode, fmt.Errorf("host Pi-hole gateway resolve returned empty dns/client IP")
	}
	return reach, "", nil
}

func resolvePiHoleTargetXrayContainer(server *models.VPNServer, preferred string) string {
	preferred = strings.TrimSpace(preferred)
	if preferred == "" {
		preferred = defaultXrayContainer
	}

	runtime := resolveXrayRuntimeLocation(server, preferred)
	if strings.TrimSpace(runtime.container) != "" {
		return strings.TrimSpace(runtime.container)
	}

	for _, candidate := range candidateXrayContainers(preferred) {
		if candidate == "" {
			continue
		}
		if out, err := sshExec(server, fmt.Sprintf(`docker ps -a --format '{{.Names}}' | grep -qx %s && echo ok`, shellQuote(candidate))); err == nil && strings.Contains(out, "ok") {
			return candidate
		}
	}

	return preferred
}

func syncPiHoleServer(server *models.VPNServer, targetXrayContainer string) piHoleSyncResult {
	result := piHoleSyncResult{
		ServerID:   server.ID,
		ServerName: server.Name,
	}

	if !server.PiHoleEnabled || server.PiHoleMode == piholeDisabled {
		result.ErrorCode = piHoleErrDisabled
		result.Error = "Pi-hole disabled on this server"
		return result
	}

	if strings.TrimSpace(server.SSHPassword) == "" {
		result.ErrorCode = piHoleErrSSHRequired
		result.Error = "SSH password not configured"
		return result
	}

	targetXrayContainer = resolvePiHoleTargetXrayContainer(server, targetXrayContainer)

	mode, containerName, detectErr := detectPiHole(server)
	if detectErr != nil {
		log.Printf("[Pi-hole] detect error on %s: %v", server.Name, detectErr)
	}

	if mode == "" {
		switch server.PiHoleMode {
		case piholeDocker, piholeAuto:
			log.Printf("[Pi-hole] deploying Docker Pi-hole on %s", server.Name)
			var err error
			containerName, err = deployDockerPiHole(server)
			if err != nil {
				result.ErrorCode = piHoleErrDeployFailed
				result.Error = fmt.Sprintf("Docker Pi-hole deploy failed: %v", err)
				return result
			}
			mode = piholeDocker
		default:
			result.ErrorCode = piHoleErrNotFound
			result.Error = "Pi-hole not found and auto-deploy not enabled"
			return result
		}
	}

	result.ModeDetected = mode

	var reach piHoleReachability
	var errorCode string
	var err error
	switch mode {
	case piholeDocker:
		reach, errorCode, err = ensurePiHoleDockerReachability(server, containerName, targetXrayContainer)
	case piholeHost:
		reach, errorCode, err = resolveVIPHostPiHoleGateway(server, targetXrayContainer)
	default:
		result.ErrorCode = piHoleErrNotFound
		result.Error = "Pi-hole mode could not be resolved"
		return result
	}
	if err != nil {
		result.ErrorCode = errorCode
		result.Error = err.Error()
		return result
	}

	result.DNSIP = reach.DNSIP
	result.ClientIP = reach.ClientIP
	result.DNSSource = reach.DNSSource
	result.XrayReachable = reach.XrayReachable

	dbInfo, errorCode, err := inspectPiHoleDB(server, mode, containerName)
	result.DBPath = dbInfo.Path
	if err != nil {
		result.ErrorCode = errorCode
		result.Error = err.Error()
		return result
	}

	if missing := missingPiHoleTables(dbInfo.Tables); len(missing) > 0 {
		if mode == piholeHost {
			if recoverErr := attemptRecoverPiHoleGravityDB(server, mode, containerName); recoverErr != nil {
				result.ErrorCode = piHoleErrRecoverFailed
				result.Error = fmt.Sprintf("Pi-hole gravity recovery failed for %s: %v", server.Name, recoverErr)
				return result
			}

			dbInfo, errorCode, err = inspectPiHoleDB(server, mode, containerName)
			result.DBPath = dbInfo.Path
			if err != nil {
				result.ErrorCode = errorCode
				result.Error = err.Error()
				return result
			}
			missing = missingPiHoleTables(dbInfo.Tables)
		}
		if len(missing) == 0 {
			goto seedPiHoleBaseConfig
		}

		result.ErrorCode = piHoleErrSchemaMismatch
		result.Error = fmt.Sprintf("Pi-hole DB schema mismatch for %s: db=%s missing=%s tables=%s",
			server.Name, dbInfo.Path, strings.Join(missing, ","), strings.Join(dbInfo.Tables, ","))
		return result
	}

seedPiHoleBaseConfig:
	listsSeeded, clientSynced, errorCode, err := ensurePiHoleBaseConfig(server, mode, containerName, dbInfo.Path, result.ClientIP)
	if err != nil {
		result.ErrorCode = errorCode
		result.Error = err.Error()
		return result
	}

	result.GroupSynced = true
	result.ListsSynced = listsSeeded
	result.ClientSynced = clientSynced
	return result
}

func persistPiHoleSyncResult(db *gorm.DB, server *models.VPNServer, result piHoleSyncResult) error {
	if db == nil || server == nil {
		return nil
	}

	now := time.Now()
	updates := map[string]interface{}{
		"pi_hole_last_sync_at":    now,
		"pi_hole_last_sync_error": result.Error,
	}
	if result.ModeDetected != "" {
		updates["pi_hole_last_mode"] = result.ModeDetected
	}
	if result.ClientIP != "" {
		updates["pi_hole_last_client_ip"] = result.ClientIP
	}
	if result.Error == "" && result.DNSIP != "" {
		updates["pi_hole_dns_ip"] = result.DNSIP
		updates["pi_hole_last_sync_error"] = ""
	}

	if err := db.Model(server).Updates(updates).Error; err != nil {
		return err
	}

	server.PiHoleLastSyncAt = &now
	server.PiHoleLastSyncError = result.Error
	if result.ModeDetected != "" {
		server.PiHoleLastMode = result.ModeDetected
	}
	if result.ClientIP != "" {
		server.PiHoleLastClientIP = result.ClientIP
	}
	if result.Error == "" && result.DNSIP != "" {
		server.PiHoleDNSIP = result.DNSIP
	}
	return nil
}
