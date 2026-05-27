if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install --install-recommends"; what_pkg="-s install"; check_pkgs="-yq update"; docker_pkg="docker.io"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; what_pkg="--assumeno install --setopt=tsflags=test"; check_pkgs="-yq check-update"; docker_pkg="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; what_pkg="--assumeno install --setopt=tsflags=test"; check_pkgs="-y -q check-update"; docker_pkg="docker"; dist="centos";\
elif which zypper > /dev/null 2>&1; then pm=$(which zypper); silent_inst="-nq install"; what_pkg="--dry-run install"; check_pkgs="-nq refresh"; docker_pkg="docker"; dist="suse";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="-S --noconfirm --noprogressbar --quiet"; what_pkg="-Sp"; check_pkgs="-Sup"; docker_pkg="docker"; dist="archlinux";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, What pkg command: $what_pkg, Check pkgs command: $check_pkgs, Docker pkg: $docker_pkg, Language: $LANG";\
echo $LANG | grep -qE '^(en_US.UTF-8|C.UTF-8|C)$' || export LC_ALL=C;\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo; fi;\
if ! sudo sh -c 'command -v fuser > /dev/null 2>&1'; then sudo $pm $check_pkgs; sudo $pm $silent_inst psmisc; fi;\
if ! sudo sh -c 'command -v lsof > /dev/null 2>&1'; then sudo $pm $check_pkgs; sudo $pm $silent_inst lsof; fi;\
if ! sudo sh -c 'command -v docker > /dev/null 2>&1'; then \
  sudo $pm $check_pkgs;\
  if ! sudo $pm $what_pkg $docker_pkg 2>/dev/null | grep -qi podman; then \
    sudo $pm $silent_inst $docker_pkg;\
    sleep 5; sudo systemctl enable --now docker; sleep 5;\
  else \
    echo "Containerization app is not supported";\
    exit 1;\
  fi;\
fi;\
if [ "$(sudo cat /sys/module/apparmor/parameters/enabled 2>/dev/null)" = "Y" ]; then \
  if ! sudo sh -c 'command -v apparmor_parser > /dev/null 2>&1'; then \
    sudo $pm $check_pkgs; sudo $pm $silent_inst apparmor;\
  fi;\
fi;\
if [ "$(sudo systemctl is-active docker)" != "active" ]; then \
  sleep 5; sudo systemctl start docker; sleep 5;\
  if [ "$(sudo systemctl is-active docker)" != "active" ]; then echo "Service status not active"; fi;\
fi;\
sudo docker --version;\
uname -sr
