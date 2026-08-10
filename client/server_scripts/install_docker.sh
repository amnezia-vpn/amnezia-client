if pm=$(which apt-get 2>/dev/null || command -v apt-get 2>/dev/null); then silent_inst="-yq install --install-recommends"; what_pkg="-s install"; check_pkgs="-yq update"; docker_pkg="docker.io"; dist="debian";\
elif pm=$(which dnf 2>/dev/null || command -v dnf 2>/dev/null); then silent_inst="-yq install"; what_pkg="--assumeno install --setopt=tsflags=test"; check_pkgs="-yq check-update"; docker_pkg="docker"; dist="fedora";\
elif pm=$(which yum 2>/dev/null || command -v yum 2>/dev/null); then silent_inst="-y -q install"; what_pkg="--assumeno install --setopt=tsflags=test"; check_pkgs="-y -q check-update"; docker_pkg="docker"; dist="centos";\
elif pm=$(which zypper 2>/dev/null || command -v zypper 2>/dev/null); then silent_inst="-nq install"; what_pkg="--dry-run install"; check_pkgs="-nq refresh"; docker_pkg="docker"; dist="suse";\
elif pm=$(which pacman 2>/dev/null || command -v pacman 2>/dev/null); then silent_inst="-S --noconfirm --noprogressbar --quiet"; what_pkg="-Sp"; check_pkgs="-Sup"; docker_pkg="docker"; dist="archlinux";\
fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, What pkg command: $what_pkg, Check pkgs command: $check_pkgs, Docker pkg: $docker_pkg, Language: $LANG";\
echo $LANG | grep -qE '^(en_US.UTF-8|C.UTF-8|C)$' || export LC_ALL=C;\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo; fi;\
if ! sudo -n sh -c 'command -v which > /dev/null 2>&1'; then sudo -n $pm $check_pkgs; sudo -n $pm $silent_inst which; fi;\
if ! sudo -n sh -c 'command -v fuser > /dev/null 2>&1'; then sudo -n $pm $check_pkgs; sudo -n $pm $silent_inst psmisc; fi;\
if ! sudo -n sh -c 'command -v lsof > /dev/null 2>&1'; then sudo -n $pm $check_pkgs; sudo -n $pm $silent_inst lsof; fi;\
if ! sudo -n sh -c 'command -v docker > /dev/null 2>&1'; then \
  sudo -n $pm $check_pkgs;\
  if ! sudo -n $pm $what_pkg $docker_pkg 2>/dev/null | grep -qi podman; then \
    sudo -n $pm $silent_inst $docker_pkg;\
    sleep 5; sudo -n systemctl enable --now docker; sleep 5;\
  else \
    echo "Container runtime is not supported";\
    exit 1;\
  fi;\
fi;\
if [ "$(sudo -n cat /sys/module/apparmor/parameters/enabled 2>/dev/null)" = "Y" ]; then \
  if ! sudo -n sh -c 'command -v apparmor_parser > /dev/null 2>&1'; then \
    sudo -n $pm $check_pkgs; sudo -n $pm $silent_inst apparmor;\
  fi;\
fi;\
if [ "$(sudo -n systemctl is-active docker)" != "active" ]; then \
  sleep 5; sudo -n systemctl start docker; sleep 5;\
  if [ "$(sudo -n systemctl is-active docker)" != "active" ]; then echo "Container runtime service not running"; fi;\
fi;\
sudo -n docker --version || docker --version;\
uname -sr
