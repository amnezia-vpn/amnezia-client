if command -v apt-get > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif command -v dnf > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/cache/dnf/* /var/run/dnf/* /var/lib/dnf/* /var/lib/rpm/*";\
elif command -v yum > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/yum.pid";\
elif command -v zypper > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/zypp.pid";\
elif command -v pacman > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/pacman/db.lck";\
else echo "Packet manager not found"; echo "Internal error"; exit 1;\
fi;\
if ! sudo -n which $LOCK_CMD > /dev/null 2>&1 && ! command -v $LOCK_CMD > /dev/null 2>&1; then echo "$LOCK_CMD not installed"; exit 0; fi;\
TIMEOUT_CMD=""; if command -v timeout > /dev/null 2>&1; then TIMEOUT_CMD="timeout 15"; fi;\
if [ "$LOCK_CMD" = "cat" ]; then if sudo -n cat $LOCK_FILE > /dev/null 2>&1; then echo "SERVER_BUSY"; fi; else if $TIMEOUT_CMD sudo -n fuser -s $LOCK_FILE > /dev/null 2>&1; then echo "SERVER_BUSY"; fi; fi
