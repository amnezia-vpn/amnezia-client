if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install"; check_pkgs="-yq update"; what_pkg="-s install"; docker_pkg="docker.io"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; check_pkgs="-yq check-update"; what_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; check_pkgs="-y -q check-update"; what_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; dist="centos";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="-S --noconfirm --noprogressbar --quiet"; check_pkgs="-Sup"; what_pkg="-Sp"; docker_pkg="docker"; dist="archlinux";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, Check pkgs command: $check_pkgs, What pkg command: $what_pkg, Docker pkg: $docker_pkg";\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
CHECK_LOCALE=$(locale | grep -c 'LANG=en_US.UTF-8');\
if [ "$CHECK_LOCALE" = "0" ]; then \
  CHECK_LOCALE=$(locale | grep -c 'LANG=C.UTF-8');\
  if [ "$CHECK_LOCALE" = "0" ]; then \
    CHECK_LOCALE=$(locale -a | grep -c 'en_US.utf8');\
    if [ "$CHECK_LOCALE" != "0" ]; then export LC_ALL=en_US.UTF-8;\
    else \
      CHECK_LOCALE=$(locale -a | grep 'C.utf8');\
      if [ "$CHECK_LOCALE" != "0" ]; then export LC_ALL=C.UTF-8; fi;\
    fi;\
  fi;\
fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo; fi;\
if ! command -v fuser > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst psmisc; fi;\
if ! command -v lsof > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst lsof; fi;\
if ! command -v docker > /dev/null 2>&1; then sudo $pm $check_pkgs;\
  check_podman=$(sudo $pm $what_pkg $docker_pkg 2>&1 | grep -c podman-docker);\
  check_moby=$(sudo $pm $what_pkg $docker_pkg 2>&1 | grep -c moby-engine);\
  if [ "$check_podman" != "0" ] || [ "$check_moby" != "0" ]; then echo "Container is not supported"; exit 1;\
  else sudo $pm $silent_inst $docker_pkg;\
    if ! command -v docker > /dev/null 2>&1; then docker; exit 1;\
    else sleep 5; sudo systemctl enable --now docker; sleep 5; fi;\
  fi;\
fi;\
if [ "$(systemctl is-active docker)" != "active" ]; then \
  sudo $pm $check_pkgs; sudo $pm $silent_inst $docker_pkg;\
  sleep 5; sudo systemctl start docker; sleep 5;\
fi;\
if ! command -v docker > /dev/null 2>&1; then echo "Failed to install docker, command not found"; exit 1; fi;\
docker --version
