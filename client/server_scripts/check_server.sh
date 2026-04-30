#!/bin/sh

LOG_DATE=$(date -u +'%Y%m%d-%H%M%S')
SCRIPT_DIR=$(dirname "$0")
LOG_FILE="${SCRIPT_DIR}/server-diagnostics-${LOG_DATE}.log"

# Logging function (sh compatible)
log_and_display() {
    if [ "$1" = "-n" ]; then
        shift
        printf "%s" "$*" | tee -a "$LOG_FILE"
    else
        echo "$1" | tee -a "$LOG_FILE"
    fi
}

# Redirect stderr to stdout for logging
exec 2>&1

header() {
    log_and_display ""
    log_and_display "=== $1 ==="
}

# Pause for cancellation
log_and_display ""
log_and_display "VPN Server Diagnostics will start in 9s. Press Ctrl+C to cancel."
sleep 9

log_and_display ""
header "STARTING VPN SERVER DIAGNOSTICS"
log_and_display ""

# ------------------------------------------------------------------------------
# 1. Basic system information
# ------------------------------------------------------------------------------
header "System Information"

# Uptime
UPTIME_STR=$(awk '{printf "%d:%02d:%02d", int($1/3600), int(($1%3600)/60), int($1%60)}' /proc/uptime 2>/dev/null || echo "unknown")
    log_and_display "Uptime (H:M:S): $UPTIME_STR"

# Date/time UTC
DATE_UTC=$(date -u +'%d %b %Y|%T' 2>/dev/null || echo "unknown")
    log_and_display "Date|Time (UTC): $DATE_UTC"

# Init system (PID 1)
INIT_NAME=$(cat /proc/1/status 2>/dev/null | head -1 | awk '{print $2}' 2>/dev/null || echo "unknown")
    log_and_display "Init system (PID 1): $INIT_NAME"

# Locale
if echo "$LANG" | grep -E '^(en_US.UTF-8|C.UTF-8|C)$' >/dev/null 2>&1; then
  log_and_display "Locale: $LANG"
else
  log_and_display "Locale: $LANG (not en_US.UTF-8, C.UTF-8 or C)"
fi

# ------------------------------------------------------------------------------
# 2. Package manager detection
# ------------------------------------------------------------------------------
header "Package Manager Information"

if command -v apt-get >/dev/null 2>&1; then
  log_and_display "Package Manager: APT"
  PM="apt-get"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker.io"
elif command -v dnf >/dev/null 2>&1; then
  log_and_display "Package Manager: DNF"
  PM="dnf"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker"
elif command -v yum >/dev/null 2>&1; then
  log_and_display "Package Manager: YUM"
  PM="yum"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker"
elif command -v zypper >/dev/null 2>&1; then
  log_and_display "Package Manager: ZYPPER"
  PM="zypper"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker"
elif command -v pacman >/dev/null 2>&1; then
  log_and_display "Package Manager: PACMAN"
  PM="pacman"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker"
elif command -v opkg >/dev/null 2>&1; then
  log_and_display "Package Manager: OPKG - Not supported on this platform"
  PM="opkg"
  PM_VER_OPT="--version"
  DOCKER_PKG="docker"
else
  log_and_display "Package Manager: Unknown"
  # fallback
  PM="uname"
  PM_VER_OPT="-a"
  DOCKER_PKG="docker"
fi

# Check package versions
log_and_display ""
log_and_display "Package versions:"

# Check sudo
if [ "$PM" = "apt-get" ]; then
  sudo_version=$(dpkg -s "sudo" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
elif [ "$PM" = "dnf" ] || [ "$PM" = "yum" ] || [ "$PM" = "zypper" ]; then
  sudo_version=$(rpm -q "sudo" 2>/dev/null || echo "not installed")
elif [ "$PM" = "pacman" ]; then
  sudo_version=$(pacman -Q "sudo" 2>/dev/null || echo "not installed")
elif [ "$PM" = "opkg" ]; then
  sudo_version=$(opkg info "sudo" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
else
  sudo_version="unknown"
fi
log_and_display "  sudo: $sudo_version"

# Check Docker package
if [ "$PM" = "apt-get" ]; then
  docker_pkg_version=$(dpkg -s "$DOCKER_PKG" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
elif [ "$PM" = "dnf" ] || [ "$PM" = "yum" ] || [ "$PM" = "zypper" ]; then
  docker_pkg_version=$(rpm -q "$DOCKER_PKG" 2>/dev/null || echo "not installed")
elif [ "$PM" = "pacman" ]; then
  docker_pkg_version=$(pacman -Q "$DOCKER_PKG" 2>/dev/null || echo "not installed")
elif [ "$PM" = "opkg" ]; then
  docker_pkg_version=$(opkg info "$DOCKER_PKG" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
else
  docker_pkg_version="unknown"
fi
log_and_display "  $DOCKER_PKG: $docker_pkg_version"

# Check lsof
if [ "$PM" = "apt-get" ]; then
  lsof_version=$(dpkg -s "lsof" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
elif [ "$PM" = "dnf" ] || [ "$PM" = "yum" ] || [ "$PM" = "zypper" ]; then
  lsof_version=$(rpm -q "lsof" 2>/dev/null || echo "not installed")
elif [ "$PM" = "pacman" ]; then
  lsof_version=$(pacman -Q "lsof" 2>/dev/null || echo "not installed")
elif [ "$PM" = "opkg" ]; then
  lsof_version=$(opkg info "lsof" 2>/dev/null | grep '^Version:' | awk '{print $2}' || echo "not installed")
else
  lsof_version="unknown"
fi
log_and_display "  lsof: $lsof_version"

# ------------------------------------------------------------------------------
# 3. Additional system information (hostnamectl / /proc/version)
# ------------------------------------------------------------------------------
header "OS / Kernel Information"

if command -v hostnamectl >/dev/null 2>&1; then
  hostnamectl 2>/dev/null | grep -E 'Operating System:|Virtualization:|Kernel:|Architecture:' | sed 's/^[ \t]*//;s/:/: /' | while read line; do
    log_and_display "  $line"
  done
else
  log_and_display "Operating System: $(cat /proc/version 2>/dev/null || echo 'unknown')"
fi

# CPU threads
CPU_THREADS=$(nproc 2>/dev/null || grep -c "^processor" /proc/cpuinfo 2>/dev/null || echo "unknown")
log_and_display "  CPU threads: $CPU_THREADS"

# ------------------------------------------------------------------------------
# 4. Memory (RAM) check
# ------------------------------------------------------------------------------
header "Memory Information"

if command -v free >/dev/null 2>&1; then
  # Remove extra spaces in header
  free -h 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error getting memory info"
elif command -v vmstat >/dev/null 2>&1; then
  vmstat -S M -s 2>/dev/null | grep -iE 'total memory|total swap' | sed 's/ *//' | tee -a "$LOG_FILE" || log_and_display "  Error getting memory info"
else
  grep -iE 'MemTotal|SwapTotal' /proc/meminfo 2>/dev/null | sed 's/ \+/ /' | tee -a "$LOG_FILE" || log_and_display "  Error getting memory info"
fi

if command -v free >/dev/null 2>&1; then
  log_and_display ""
  log_and_display "Detailed Memory Info:"
  free -h 2>/dev/null | awk 'NR==2{printf "  Used: %s / %s (%.1f%%)\n", $3, $2, $3/$2*100}' 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error calculating memory usage"
free -h 2>/dev/null | awk 'NR==3{printf "  Swap: %s / %s (%.1f%%)\n", $3, $2, $2>0 ? $3/$2*100 : 0}' 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error calculating swap usage"
fi

# Disk usage
header "Disk Usage"
df -h 2>/dev/null | awk '
BEGIN {print "  Filesystem   Size  Used Avail Use% Mounted"}
NR>1 {printf "  %-10s %5s %5s %5s %4s %s\n", $1, $2, $3, $4, $5, $6}' | tee -a "$LOG_FILE" || log_and_display "  Error getting disk usage"

# ------------------------------------------------------------------------------
# 5. Current user and sudo check
# ------------------------------------------------------------------------------
header "User Check"

CUR_USER=$(whoami 2>/dev/null || echo ~ | sed 's/.*\///')
USER_GROUP=$(groups "$CUR_USER" 2>/dev/null || echo "")
USER_GOOD=0

log_and_display -n "Current user: $CUR_USER => "

if [ "$CUR_USER" = "root" ]; then
  log_and_display "passed.. (is root)"
  USER_GOOD="r" # root
else
  if echo "$USER_GROUP" | grep -qE '(^|[[:space:]])sudo($|[[:space:]])'; then
    log_and_display "passed.. (in sudo group)"
    USER_GOOD=1
  elif echo "$USER_GROUP" | grep -qE '(^|[[:space:]])wheel($|[[:space:]])'; then
    log_and_display "passed.. (in wheel group)"
    USER_GOOD=1
  elif echo "$USER_GROUP" | grep -qE '(^|[[:space:]])docker($|[[:space:]])'; then
    log_and_display "failed.. (only in docker group)"
    USER_GOOD="d"
  else
    log_and_display "failed.. (not a member of the sudo or wheel groups)"
    USER_GOOD=0
  fi
fi

# Check if password is required for sudo
if [ "$USER_GOOD" = "0" ] || [ "$USER_GOOD" = "d" ]; then
  log_and_display -n "Passwd request: "
  log_and_display "check skipped (not sudoer)"
else
  if command -v sudo >/dev/null 2>&1; then
    # Try sudo without password - more thorough check
    PASSWD_REQUEST=$(sudo -K 2>&1 && sudo -nu $CUR_USER $PM $PM_VER_OPT 2>&1 >/dev/null && sudo -n $PM $PM_VER_OPT 2>&1 >/dev/null)
    if [ -n "$PASSWD_REQUEST" ]; then
      USER_GOOD=0
      log_and_display -n "Passwd request: "
      log_and_display "failed.. ($PASSWD_REQUEST)" \
        | sed "s/$CUR_USER/User/g;s/$(hostname 2>/dev/null || echo 'Server')/Server/g;s/ user / /g"
    else
      log_and_display -n "Passwd request: "
      log_and_display "passed.. (not required)"
    fi
  else
    if [ "$USER_GOOD" = "r" ]; then
      log_and_display -n "Passwd request: "
      log_and_display "check skipped (sudo not installed, but root user)"
    else
      log_and_display "Warning! The sudo package must be pre-installed!"
      USER_GOOD=0
    fi
  fi
fi

# Home directory check
log_and_display -n "Home dir: "
if cd ~ 2>/dev/null; then
  log_and_display "passed.. (accessible)"
else
  log_and_display "failed.. (not accessible)"
fi
log_and_display "Default shell: $SHELL"

# ------------------------------------------------------------------------------
# 6. Important components check (sudo, lsof, fuser, apparmor)
# ------------------------------------------------------------------------------
header "Component Checks"

log_and_display -n "    sudo: "
if command -v sudo >/dev/null 2>&1; then
  log_and_display "passed.. (installed)"
else
  log_and_display "not installed"
fi

log_and_display -n "    lsof: "
if command -v lsof >/dev/null 2>&1; then
  log_and_display "passed.. (installed)"
else
  log_and_display "not installed"
fi

log_and_display -n "   fuser: "
if command -v fuser >/dev/null 2>&1; then
  log_and_display "passed.. (installed)"
else
  log_and_display "psmisc not installed"
fi

log_and_display -n "apparmor: "
AA_ENABLED=$(cat /sys/module/apparmor/parameters/enabled 2>/dev/null || echo "N")
if [ "$AA_ENABLED" = "Y" ]; then
  if command -v apparmor_parser >/dev/null 2>&1; then
    log_and_display "passed.. (used)"
  else
    log_and_display "failed.. (installation required)"
  fi
else
  if command -v apparmor_parser >/dev/null 2>&1; then
    log_and_display "passed.. (not used)"
  else
    log_and_display "passed.. (not required)"
  fi
fi

# ------------------------------------------------------------------------------
# 7. SELinux check
# ------------------------------------------------------------------------------
header "SELinux Check"

if command -v getenforce >/dev/null 2>&1; then
  SELINUX_STATUS=$(getenforce 2>/dev/null || echo "unknown")
  if [ "$SELINUX_STATUS" = "Enforcing" ]; then
    log_and_display "SELinux status: $SELINUX_STATUS (strict mode)"
  elif [ "$SELINUX_STATUS" = "Permissive" ]; then
    log_and_display "SELinux status: $SELINUX_STATUS (permissive mode)"
  else
    log_and_display "SELinux status: $SELINUX_STATUS (disabled)"
  fi
else
  log_and_display "SELinux: not found (or not applicable)"
fi

# ------------------------------------------------------------------------------
# 8. Docker + Docker/Podman service check
# ------------------------------------------------------------------------------
header "Docker / Podman Status"
CHECK_CONTAINERS=0

if ! command -v docker >/dev/null 2>&1; then
  log_and_display "Docker: $DOCKER_PKG not installed"
else
  # If user is in sudoers, use sudo without password
  if [ "$USER_GOOD" = "1" ]; then
    SUD="sudo -n"
  elif [ "$USER_GOOD" = "r" ]; then
    SUD="" # root
  else
    SUD=""
  fi

  DOCKER_VERSION=$($SUD docker -v 2>/dev/null || echo 'docker -v error')
  log_and_display "Installed: $DOCKER_VERSION"

  # Check for podman
  if echo "$DOCKER_VERSION" | grep -qi "podman"; then
    log_and_display "  WARNING: Podman detected - not supported at the moment!"
    log_and_display "  Podman (podman-docker) is not supported and is installed by mistake"
    docker_service="podman.socket"
  else
    docker_service="docker.service"
  fi
  log_and_display "  service: $docker_service"

  # Check status
  if command -v systemctl >/dev/null 2>&1; then
    docker_status=$(systemctl is-active "$docker_service" 2>/dev/null || echo "unknown")
    docker_loading=$(systemctl is-enabled "$docker_service" 2>/dev/null || echo "unknown")
  else
    docker_status="unknown (systemctl not found)"
    docker_loading="unknown"
  fi
  
  if [ "$docker_status" = "active" ]; then
    log_and_display "   status: passed.. ($docker_status)"
    CHECK_CONTAINERS=1
  else
    log_and_display "   status: incorrect.. ($docker_status)"
    CHECK_CONTAINERS=0
  fi

  if [ "$docker_loading" = "enabled" ]; then
    log_and_display "  loading: good (startup $docker_loading)"
  else
    log_and_display "  loading: bad (startup $docker_loading)"
  fi
fi

# ------------------------------------------------------------------------------
# 9. Docker pull test + container check with improved Docker Hub verification
# ------------------------------------------------------------------------------
header "Docker Hub: pull hello-world test"

if [ "$CHECK_CONTAINERS" = "1" ] && [ "$USER_GOOD" != "0" ]; then
  # First check Docker Hub availability
  log_and_display "Checking Docker Hub connectivity..."
  
  # Try to execute docker pull with timeout
  if timeout 30 $SUD docker pull docker.io/library/hello-world >/dev/null 2>&1; then
    log_and_display "Docker Hub: available"
    
    # Start container for testing
    if $SUD docker run --rm docker.io/library/hello-world >/dev/null 2>&1; then
      log_and_display "Hello-world container: successfully started and completed"
    else
      log_and_display "Hello-world container: startup error"
    fi
  else
    log_and_display "Docker Hub: unavailable or blocked (possibly exceeded download limit)"
    log_and_display "Docker Hub has download limits, try again later"
  fi
  
  log_and_display ""
  total_cont=$($SUD docker ps -aq 2>/dev/null | wc -l || echo "0")
  active_cont=$($SUD docker ps -q 2>/dev/null | wc -l || echo "0")
  amnezia_cont=$($SUD docker ps -a 2>/dev/null | grep -c amnezia || echo "0")

  log_and_display "Containers check: Total $total_cont / Active $active_cont / Amnezia $amnezia_cont"
  $SUD docker ps -a --format "{{.Names}} ({{.Image}}) ({{.Status}}) ({{.Ports}})" 2>/dev/null | grep amnezia || true
  
  # Peers check
  if $SUD docker ps 2>/dev/null | grep -qE '\<(amnezia-awg|amnezia-wireguard)\>'; then
    log_and_display ""
    log_and_display "Peers check (beta):"
    if $SUD docker ps 2>/dev/null | grep -q amnezia-awg; then
      AMNEZIA_WG_CONTAINER=$($SUD docker ps 2>/dev/null | grep amnezia-awg | awk '{print $1}' | head -1)
      if [ -n "$AMNEZIA_WG_CONTAINER" ]; then
        WG_PEERS=$($SUD docker exec -it "$AMNEZIA_WG_CONTAINER" wg show 2>/dev/null | grep -c 'peer' || echo "0")
        log_and_display "AmneziaWG peers: $WG_PEERS"
      fi
    fi
    if $SUD docker ps 2>/dev/null | grep -q amnezia-wireguard; then
      WIREGUARD_CONTAINER=$($SUD docker ps 2>/dev/null | grep amnezia-wireguard | awk '{print $1}' | head -1)
      if [ -n "$WIREGUARD_CONTAINER" ]; then
        WG_PEERS=$($SUD docker exec -it "$WIREGUARD_CONTAINER" wg show 2>/dev/null | grep -c 'peer' || echo "0")
        log_and_display "WireGuard peers: $WG_PEERS"
      fi
    fi
  fi
else
  log_and_display "skipped.."
fi

# ------------------------------------------------------------------------------
# 10. Additional improvements
# ------------------------------------------------------------------------------
#
# 10.1. CPU and memory load check (Load average, top processes)
#
header "CPU & Memory usage (top)"

# Load average (last 1,5,15 minutes)
LOAD_AVG=$(uptime 2>/dev/null | awk -F'load average:' '{print $2}' || echo "unknown")
log_and_display "Load average: $LOAD_AVG"

log_and_display ""
log_and_display "Top 5 processes by CPU:"
ps aux 2>/dev/null | sort -k3 -nr | head -n 6 | awk '{printf "%s %s %s %s %s\n", $1,$2,$3"%",$4"%",$11}' | column -t 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error getting CPU processes"

log_and_display ""
log_and_display "Top 5 processes by MEM:"
ps aux 2>/dev/null | sort -k4 -nr | head -n 6 | awk '{printf "%s %s %s %s %s\n", $1,$2,$3"%",$4"%",$11}' | column -t 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error getting MEM processes"

# 10.2. System logs check (latest critical messages)
header "Last 10 critical/error messages (journalctl)"

if command -v journalctl >/dev/null 2>&1; then
  journalctl -p 3 -n 10 --no-pager 2>/dev/null | tee -a "$LOG_FILE" || log_and_display "  Error getting system logs"
else
  log_and_display "journalctl not found (non-systemd system?)"
fi

# 10.3. System package versions check (examples)

# Open ports check
header "Network Ports Check"
if command -v netstat >/dev/null 2>&1; then
  log_and_display "Listening ports:"
  netstat -tlnp 2>/dev/null | grep LISTEN | head -10 | while read line; do
    log_and_display "  $line"
  done
elif command -v ss >/dev/null 2>&1; then
  log_and_display "Listening ports:"
  ss -tlnp 2>/dev/null | head -10 | while read line; do
    log_and_display "  $line"
  done
else
  log_and_display "netstat/ss not found"
fi

# SSH check
header "SSH Service Check"
if command -v systemctl >/dev/null 2>&1; then
  ssh_status=$(systemctl is-active ssh 2>/dev/null || systemctl is-active sshd 2>/dev/null || echo "not found")
  if [ "$ssh_status" = "active" ]; then
    log_and_display "SSH service: $ssh_status"
  else
    log_and_display "SSH service: $ssh_status"
  fi
else
  log_and_display "systemctl not found"
fi

# Time check
header "Time Synchronization"
if command -v timedatectl >/dev/null 2>&1; then
  timedatectl status 2>/dev/null | grep -E "System clock|NTP service" | while read line; do
    log_and_display "  $line"
  done
else
  log_and_display "  System time: $(date 2>/dev/null || echo 'unknown')"
fi

# Kernel check
header "Kernel Information"
log_and_display "Kernel version: $(uname -r 2>/dev/null || echo 'unknown')"
log_and_display "Kernel architecture: $(uname -m 2>/dev/null || echo 'unknown')"
if [ -f /proc/cmdline ]; then
  log_and_display "Kernel parameters:"
  cat /proc/cmdline 2>/dev/null | tr ' ' '\n' | head -5 | while read param; do
    log_and_display "  $param"
  done
fi

# ------------------------------------------------------------------------------
# Completion
# ------------------------------------------------------------------------------
log_and_display ""
header "FINISH"
log_and_display ""
log_and_display "Diagnostics completed. Log saved to: $LOG_FILE"
log_and_display ""

# Variable cleanup
pm="" && opt="" && docker_pkg="" && CUR_USER="" && USER_GOOD="" && USER_GROUP="" && PASSWD_REQUEST="" && CHECK_CONTAINERS="" && SUD="" && docker_service="" && docker_status="" && docker_loading="" 