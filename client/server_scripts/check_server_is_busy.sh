# The client sends this script in chunks: lines are glued together while they end with a
# backslash, and every resulting chunk is executed by its own remote shell.  Keep all plain
# lines (including comments) outside the backslash chain: one unescaped line in the middle
# splits the script into independent shells, breaks the if/fi nesting and loses LOCK_CMD,
# LOCK_FILE and TIMEOUT_CMD before they are used.
#
# The script must always answer with exactly one marker: SERVER_BUSY while the package
# manager lock is held, or while we cannot be sure that it is not, and SERVER_FREE only when
# it is certain that no package manager is running.  The client reads a missing or ambiguous
# answer as "not free" and retries, so an error can never let an installation run on top of a
# running package manager (#3232).
#
# yum and zypper leave their pid file behind when they exit, so a readable file does not
# prove that a package manager is running: only report busy while the recorded pid is alive.
# kill -0 needs the same uid, hence the /proc fallback for a pid we do not own.
#
# fuser -s exits 0 only while the lock is held and 1 when no process uses the file.  124 and
# 137 mean the check did not finish in time, any other code is an error: report busy instead
# of free, so that an answer we are unsure about cannot let an install run next to a package
# manager (#3232).
if command -v apt-get > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif command -v dnf > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/cache/dnf/* /var/run/dnf/* /var/lib/dnf/* /var/lib/rpm/*";\
elif command -v yum > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/yum.pid";\
elif command -v zypper > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/zypp.pid";\
elif command -v pacman > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/pacman/db.lck";\
else echo "Packet manager not found"; echo "Internal error"; echo "SERVER_BUSY"; exit 1;\
fi;\
if ! sudo -n which $LOCK_CMD > /dev/null 2>&1 && ! command -v $LOCK_CMD > /dev/null 2>&1; then echo "$LOCK_CMD not installed"; echo "SERVER_FREE"; exit 0; fi;\
TIMEOUT_CMD=""; if command -v timeout > /dev/null 2>&1; then TIMEOUT_CMD="timeout 15"; fi;\
if [ "$LOCK_CMD" = "cat" ]; then \
LOCK_PID=$(sudo -n cat $LOCK_FILE 2> /dev/null); RC=$?; \
if [ "$RC" -ne 0 ]; then \
if [ -e "$LOCK_FILE" ]; then echo "SERVER_BUSY"; else echo "SERVER_FREE"; fi; \
else \
case "$LOCK_PID" in ''|*[!0-9]*) echo "SERVER_FREE" ;; *) if kill -0 "$LOCK_PID" 2> /dev/null || [ -d "/proc/$LOCK_PID" ]; then echo "SERVER_BUSY"; else echo "SERVER_FREE"; fi ;; esac; \
fi; \
else \
RC=0; $TIMEOUT_CMD sudo -n fuser -s $LOCK_FILE > /dev/null 2>&1 || RC=$?; \
if [ "$RC" -eq 0 ] || [ "$RC" -eq 124 ] || [ "$RC" -eq 137 ]; then echo "SERVER_BUSY"; \
elif [ "$RC" -eq 1 ]; then echo "SERVER_FREE"; \
else echo "SERVER_BUSY"; fi; \
fi
