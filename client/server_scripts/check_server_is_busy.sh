# The client sends this script in chunks: lines are glued together while they end with a
# backslash, and every resulting chunk is executed by its own remote shell.  Keep all plain
# lines (including comments) outside the backslash chain: one unescaped line in the middle
# splits the script into independent shells, breaks the if/fi nesting and loses LOCK_CMD,
# LOCK_FILE, SUDO and TIMEOUT_CMD before they are used.
#
# The script must always answer with exactly one marker: SERVER_BUSY while the package
# manager lock is held, or while we cannot be sure that it is not, and SERVER_FREE only when
# it is certain that no package manager is running.  The client reads a missing or ambiguous
# answer as "not free" and retries, so an error can never let an installation run on top of a
# running package manager (#3232).
#
# yum and zypper leave their pid file behind when they exit, so a readable file does not
# prove that a package manager is running: the recorded pid is looked up in /proc, and its
# name (comm) and command line are matched against the package managers.  A pid that cannot
# be inspected at all (hidepid, EPERM) is reported busy instead of free, so a running root
# yum cannot be missed behind a filtered /proc.  A recycled pid from a stale file belongs to
# another program and does not keep the server busy forever.
#
# fuser is never asked about a path that does not exist.  Lock files are created on demand
# and removed on exit (pacman keeps db.lck only during a transaction), while dnf has several
# lock locations and some of them do not exist on every release; an unmatched glob stays a
# literal and would make fuser fail.  So all paths are filtered first: no existing path means
# no lock holder, hence SERVER_FREE.  With at least one path fuser -s exits 0 only while the
# lock is held and 1 when no process uses the file; 124 and 137 mean the check did not finish
# in time, any other code is an error: report busy instead of free, so that an answer we are
# unsure about cannot let an install run next to a package manager (#3232).
#
# psmisc (fuser) is installed only later by install_docker.sh, so it may be missing when this
# check runs.  The existing lock paths are then matched against the open file descriptors in
# /proc/*/fd, which finds both fcntl and flock locks (flock -n alone would miss fcntl locks,
# so it is not used).  The scan needs root or a working passwordless sudo; with nothing to
# scan with, an existing lock path is reported busy, never free.
#
# sudo is only used when it is both available and needed (we are not root).  A one-shot
# "$SUDO true" probe decides whether it really works: an unusable sudo is not an answer, so
# an existing lock path is then reported busy and neither fuser nor the /proc scan are
# trusted.  A missing lock path is still free, because reading it is not required to see
# that nothing is locked.
if command -v apt-get > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif command -v dnf > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/cache/dnf/* /var/run/dnf/* /var/lib/dnf/* /var/lib/rpm/*";\
elif command -v yum > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/yum.pid";\
elif command -v zypper > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/zypp.pid";\
elif command -v pacman > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/pacman/db.lck";\
else echo "Packet manager not found"; echo "Internal error"; echo "SERVER_BUSY"; exit 1;\
fi;\
SUDO=""; if command -v sudo > /dev/null 2>&1 && [ "$(id -u)" -ne 0 ]; then SUDO="sudo -n"; fi;\
CAN_CHECK=0; if [ "$(id -u)" -eq 0 ]; then CAN_CHECK=1; elif [ -n "$SUDO" ] && $SUDO true > /dev/null 2>&1; then CAN_CHECK=1; fi;\
TIMEOUT_CMD=""; if command -v timeout > /dev/null 2>&1; then TIMEOUT_CMD="timeout 15"; fi;\
if [ "$LOCK_CMD" = "cat" ]; then \
LOCK_PID=$($SUDO cat $LOCK_FILE 2> /dev/null); RC=$?; \
if [ "$RC" -ne 0 ]; then \
if [ -e "$LOCK_FILE" ]; then echo "SERVER_BUSY"; else echo "SERVER_FREE"; fi; \
else \
BAD_PID=0; case "$LOCK_PID" in ''|*[!0-9]*) BAD_PID=1 ;; esac; \
if [ "$BAD_PID" -eq 1 ]; then echo "SERVER_FREE"; \
else \
LOCK_NAME=""; if [ -r "/proc/$LOCK_PID/comm" ]; then LOCK_NAME=$(cat "/proc/$LOCK_PID/comm" 2> /dev/null); fi; \
LOCK_ARGS=""; if [ -r "/proc/$LOCK_PID/cmdline" ] && command -v tr > /dev/null 2>&1; then LOCK_ARGS=$(tr '\000' ' ' < "/proc/$LOCK_PID/cmdline" 2> /dev/null); fi; \
LOCK_MATCH=0; case "$LOCK_NAME $LOCK_ARGS" in *yum*|*dnf*|*zypper*|*pacman*|*apt*|*dpkg*|*rpm*|*python*|*perl*) LOCK_MATCH=1 ;; esac; \
if [ "$LOCK_MATCH" -eq 1 ]; then echo "SERVER_BUSY"; \
elif [ -n "$LOCK_NAME$LOCK_ARGS" ]; then echo "SERVER_FREE"; \
elif [ "$CAN_CHECK" -eq 0 ]; then echo "SERVER_BUSY"; \
elif $SUDO sh -c "kill -0 $LOCK_PID 2> /dev/null || [ -d /proc/$LOCK_PID ]"; then echo "SERVER_BUSY"; \
else echo "SERVER_FREE"; fi; \
fi; \
fi; \
else \
EXISTING=""; for p in $LOCK_FILE; do if [ -e "$p" ]; then EXISTING="$EXISTING $p"; fi; done; \
if [ -z "$EXISTING" ]; then echo "SERVER_FREE"; \
elif [ "$CAN_CHECK" -eq 0 ]; then echo "SERVER_BUSY"; \
else \
FUSER_OK=0; if command -v fuser > /dev/null 2>&1 || { [ -n "$SUDO" ] && $SUDO which fuser > /dev/null 2>&1; }; then FUSER_OK=1; fi; \
if [ "$FUSER_OK" -eq 1 ]; then \
RC=0; $SUDO $TIMEOUT_CMD fuser -s $EXISTING > /dev/null 2>&1 || RC=$?; \
if [ "$RC" -eq 0 ] || [ "$RC" -eq 124 ] || [ "$RC" -eq 137 ]; then echo "SERVER_BUSY"; \
elif [ "$RC" -eq 1 ]; then echo "SERVER_FREE"; \
else echo "SERVER_BUSY"; fi; \
else \
SCAN='h=0; for p in "$@"; do for fd in /proc/[0-9]*/fd/*; do if [ "$(readlink "$fd" 2> /dev/null)" = "$p" ]; then h=1; break 2; fi; done; done; [ "$h" -eq 1 ]'; \
if ! command -v readlink > /dev/null 2>&1; then echo "SERVER_BUSY"; \
elif $SUDO sh -c "$SCAN" sh $EXISTING > /dev/null 2>&1; then echo "SERVER_BUSY"; \
else echo "SERVER_FREE"; fi; \
fi; \
fi; \
fi
