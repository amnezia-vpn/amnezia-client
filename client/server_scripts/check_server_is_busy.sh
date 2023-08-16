if which apt-get > /dev/null 2>&1; then LOCK_FILE="/var/lib/dpkg/lock-frontend";\
elif which dnf > /dev/null 2>&1; then LOCK_FILE="/var/run/dnf.pid";\
elif which yum > /dev/null 2>&1; then LOCK_FILE="/var/run/yum.pid";\
else echo "Packet manager not found"; echo "Internal error"; exit 1; fi;\
if command -v fuser > /dev/null 2>&1; then sudo fuser $LOCK_FILE; else  echo "Not installed"; fi
