if which apt-get > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif which dnf > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/cache/dnf/* /var/run/dnf/* /var/lib/dnf/* /var/lib/rpm/*";\
elif which yum > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/yum.pid";\
elif which zypper > /dev/null 2>&1; then LOCK_CMD="cat"; LOCK_FILE="/var/run/zypp.pid";\
elif which pacman > /dev/null 2>&1; then LOCK_CMD="fuser"; LOCK_FILE="/var/lib/pacman/db.lck";\
else echo "Packet manager not found"; echo "Internal error"; exit 1; fi;\
if command -v $LOCK_CMD > /dev/null 2>&1; then sudo $LOCK_CMD $LOCK_FILE 2>/dev/null; else echo "$LOCK_CMD not installed"; fi
