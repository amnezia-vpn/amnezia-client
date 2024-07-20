if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install"; check_pkgs="-yq update"; wh_pkg="-s install"; docker_pkg="docker.io"; check_srv="docker"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; check_pkgs="-yq check-update"; wh_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; check_srv="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; check_pkgs="-y -q check-update"; wh_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; check_srv="docker"; dist="centos";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="-S --noconfirm --noprogressbar --quiet"; check_pkgs="-Sup"; wh_pkg="-Sp"; docker_pkg="docker"; check_srv="docker"; dist="archlinux";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, Check pkgs command: $check_pkgs, What pkg command: $wh_pkg, Docker pkg: $docker_pkg, Check service: $check_srv";\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if [ -z "$(echo $LANG | grep -E 'en_US.UTF-8|C.UTF-8')" ]; then \
  if [ -n "$(locale -a | grep en_US.utf8)" ]; then export LC_ALL=en_US.UTF-8;\
  elif [ -n "$(locale -a | grep C.utf8)" ]; then export LC_ALL=C.UTF-8;\
  fi;\
fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo || sudo 2>&1 > /dev/null || exit 1; fi;\
if ! command -v fuser > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst psmisc || fuser 2>&1 > /dev/null || exit 1; fi;\
if ! command -v lsof > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst lsof || lsof 2>&1 > /dev/null || exit 1; fi;\
if ! command -v docker > /dev/null 2>&1; then sudo $pm $check_pkgs;\
  if [ -n "$(sudo $pm $wh_pkg $docker_pkg 2>/dev/null | grep moby-engine)" ]; then echo "Docker is not supported"; echo "command not found"; exit 1;\
  else sudo $pm $silent_inst $docker_pkg || docker 2>&1 > /dev/null || exit 1;\
  fi;\
  if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then check_srv="podman.socket podman"; sudo touch /etc/containers/nodocker; fi;\
  sleep 5; sudo systemctl enable --now $check_srv 2>/dev/null; sleep 5;\
fi;\
if [ -n "$(sudo docker --version 2>&1 | grep moby-engine)" ]; then echo "Docker is not supported"; echo "command not found"; exit 1;\
elif [ -n "$(sudo docker --version 2>&1 | grep podman)" ]; then check_srv="podman.socket podman"; docker_pkg="podman-docker";\
  if [ -n "$(sudo docker --version 2>&1 | grep /etc/containers/nodocker)" ]; then sudo touch /etc/containers/nodocker; fi;\
fi;\
if [ "$(systemctl is-active $check_srv | head -n1)" != "active" ]; then \
  sudo $pm $check_pkgs; sudo $pm $silent_inst $docker_pkg;\
  sleep 5; sudo systemctl start $check_srv; sleep 5;\
  if [ "$(systemctl is-active $check_srv | head -n1)" != "active" ]; then echo "Failed to status docker"; echo "command not found"; exit 1; fi;\
fi;\
sudo docker --version
