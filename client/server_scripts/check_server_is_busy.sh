pm_apt="/usr/bin/apt-get";\
if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else exit; fi;\
if [[ ! -f "/usr/bin/sudo" ]]; then $pm update -y -q; $pm install -y -q sudo; fi;\
sudo lsof /var/lib/dpkg/lock-frontend