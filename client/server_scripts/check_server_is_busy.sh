if command -v apt-get > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif command -v dnf > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/cache/dnf/* /var/run/dnf/* /var/lib/dnf/* /var/lib/rpm/*";\
elif command -v yum > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/yum.pid";\
elif command -v zypper > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/zypp.pid";\
elif command -v pacman > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/pacman/db.lck";\
else echo "Packet manager not found"; echo "Internal error"; exit 1;\
fi;\
if ! sudo -n which $LOCK_CMD > /dev/null 2>&1 && ! command -v $LOCK_CMD > /dev/null 2>&1; then echo "$LOCK_CMD not installed"; exit 0; fi;\
TIMEOUT_CMD=""; if command -v timeout > /dev/null 2>&1; then TIMEOUT_CMD="timeout 15"; fi;\
# yum and zypper leave their pid file behind when they exit, so a readable file does not
# prove that a package manager is running: only report busy while the recorded pid is alive.
# kill -0 needs the same uid, hence the /proc fallback for a pid we do not own.
if [ "$LOCK_CMD" = "cat" ]; then \
LOCK_PID=$(sudo -n cat $LOCK_FILE 2> /dev/null); \
case "$LOCK_PID" in ''|*[!0-9]*) ;; *) if kill -0 "$LOCK_PID" 2> /dev/null || [ -d "/proc/$LOCK_PID" ]; then echo "SERVER_BUSY"; fi ;; esac; \
else \
# fuser -s exits 0 only while the lock is held. 124 and 137 mean the check did not finish
# in time: report busy instead of free, so that an answer we are unsure about cannot let an
# install run next to a package manager (#3232).
RC=0; $TIMEOUT_CMD sudo -n fuser -s $LOCK_FILE > /dev/null 2>&1 || RC=$?; \
if [ "$RC" -eq 0 ] || [ "$RC" -eq 124 ] || [ "$RC" -eq 137 ]; then echo "SERVER_BUSY"; fi; \
fi
