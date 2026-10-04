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
# prove that a package manager is running: the recorded pid is looked up in /proc, and both
# its name (comm) and every word of its command line are matched as whole names against the
# package managers.  Exact matching keeps a recycled pid from holding the server busy
# forever: a stray python or perl process behind a stale yum.pid (urlgrabber-ext-down) no
# longer matches, while an interpreter-run manager is still recognized through the script
# name in its arguments (/usr/bin/python2 /usr/bin/yum).  A pid with no /proc entry while
# /proc itself is readable is gone, so a stale file does not keep the server busy; a pid that
# cannot be inspected (hidepid, EPERM) is reported busy instead of free, so a running root
# yum cannot be missed behind a filtered /proc.
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
# fuser is started as "sh -c '<cmd>; echo rc=$?'" so that its own exit code comes back in
# stdout: a sudo that fails by itself (expired timestamp, sudoers that only allow selected
# commands) leaves no rc behind any more, while its rc=1 used to look exactly like "nobody
# holds the lock".  The same wrapper is used for the /proc scan below.
#
# psmisc (fuser) is installed only later by install_docker.sh, so it may be missing when this
# check runs, and sudo may be usable only for selected commands.  The existing lock paths are
# then matched against the open file descriptors in /proc/*/fd: one pass over the
# descriptors, each compared through test -ef, so the kernel resolves the paths itself
# (/var/run -> /run, /var/lib/rpm -> /usr/lib/sysimage/rpm) and no fork is spent per
# descriptor.  The scan runs under timeout and needs no privileges, so it is used both when
# fuser is absent and when sudo is not usable, instead of a lock path that exists being
# declared busy forever; an answer the scan cannot give (killed, no /proc) is busy, never
# free.
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
if [ "$RC" -ne 0 ] && [ -n "$SUDO" ]; then LOCK_PID=$(cat $LOCK_FILE 2> /dev/null); RC=$?; fi; \
if [ "$RC" -ne 0 ]; then \
if [ -e "$LOCK_FILE" ]; then echo "SERVER_BUSY"; else echo "SERVER_FREE"; fi; \
else \
BAD_PID=0; case "$LOCK_PID" in ''|*[!0-9]*) BAD_PID=1 ;; esac; \
if [ "$BAD_PID" -eq 1 ]; then echo "SERVER_FREE"; \
else \
LOCK_NAME=""; if [ -r "/proc/$LOCK_PID/comm" ]; then LOCK_NAME=$(cat "/proc/$LOCK_PID/comm" 2> /dev/null); fi; \
LOCK_ARGS=""; if [ -r "/proc/$LOCK_PID/cmdline" ] && command -v tr > /dev/null 2>&1; then LOCK_ARGS=$(tr '\000' ' ' < "/proc/$LOCK_PID/cmdline" 2> /dev/null); fi; \
LOCK_MATCH=0; case "$LOCK_NAME" in dnf|yum|zypper|rpm|pacman|dpkg|apt|apt-get|unattended-upgrades|packagekitd) LOCK_MATCH=1 ;; esac; \
for a in $LOCK_ARGS; do case "${a##*/}" in dnf|yum|zypper|rpm|pacman|dpkg|apt|apt-get|unattended-upgrades|packagekitd) LOCK_MATCH=1 ;; esac; done; \
if [ "$LOCK_MATCH" -eq 1 ]; then echo "SERVER_BUSY"; \
elif [ -n "$LOCK_NAME$LOCK_ARGS" ]; then echo "SERVER_FREE"; \
elif [ ! -d "/proc/$LOCK_PID" ] && [ -r /proc/1/comm ]; then echo "SERVER_FREE"; \
elif [ "$CAN_CHECK" -eq 0 ]; then echo "SERVER_BUSY"; \
elif $SUDO sh -c "kill -0 $LOCK_PID 2> /dev/null || [ -d /proc/$LOCK_PID ]"; then echo "SERVER_BUSY"; \
else echo "SERVER_FREE"; fi; \
fi; \
fi; \
else \
EXISTING=""; for p in $LOCK_FILE; do if [ -e "$p" ]; then EXISTING="$EXISTING $p"; fi; done; \
if [ -z "$EXISTING" ]; then echo "SERVER_FREE"; \
else \
FUSER_RC=""; \
if [ "$CAN_CHECK" -eq 1 ]; then \
FUSER_OUT=$($SUDO $TIMEOUT_CMD sh -c "echo run=1; fuser -s $EXISTING; echo rc=\$?" 2> /dev/null); \
case "$FUSER_OUT" in *"rc="*) FUSER_RC=${FUSER_OUT##*rc=} ;; *"run=1"*) FUSER_RC=124 ;; esac; \
case "$FUSER_RC" in ''|*[!0-9]*) FUSER_RC="" ;; esac; \
fi; \
SCAN='scan() { h=0; if [ -d /proc/1 ]; then for fd in /proc/[0-9]*/fd/*; do for p in "$@"; do if [ "$fd" -ef "$p" ]; then h=1; break 2; fi; done; done; else h=2; fi; case "$h" in 1) return 0 ;; 2) return 2 ;; *) return 1 ;; esac; }; scan "$@"'; \
case "$FUSER_RC" in \
0|124|137) echo "SERVER_BUSY" ;; \
1) echo "SERVER_FREE" ;; \
126|127|'') \
SCAN_RC=3; \
if [ "$CAN_CHECK" -eq 1 ] && [ -n "$SUDO" ]; then \
SCAN_OUT=$($SUDO $TIMEOUT_CMD sh -c "echo run=1; $SCAN; echo rc=\$?" sh $EXISTING 2> /dev/null); \
case "$SCAN_OUT" in *"rc="*) SCAN_RC=${SCAN_OUT##*rc=}; case "$SCAN_RC" in ''|*[!0-9]*) SCAN_RC=3 ;; esac ;; *"run=1"*) SCAN_RC=124 ;; esac; \
fi; \
if [ "$SCAN_RC" -eq 3 ]; then $TIMEOUT_CMD sh -c "$SCAN" sh $EXISTING > /dev/null 2>&1; SCAN_RC=$?; fi; \
if [ "$SCAN_RC" -eq 1 ]; then echo "SERVER_FREE"; else echo "SERVER_BUSY"; fi ;; \
*) echo "SERVER_BUSY" ;; \
esac; \
fi; \
fi
