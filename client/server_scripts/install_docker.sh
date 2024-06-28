if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install"; check_pkgs="-yq update"; what_pkg="-s install"; docker_pkg="docker.io"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; check_pkgs="-yq check-update"; what_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; check_pkgs="-y -q check-update"; what_pkg="--assumeno install --setopt=tsflags=test"; docker_pkg="docker"; dist="centos";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="-S --noconfirm --noprogressbar --quiet"; check_pkgs="-Sup"; what_pkg="-Sp"; docker_pkg="docker"; dist="archlinux";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, Check pkgs command: $check_pkgs, What pkg command: $what_pkg, Docker pkg: $docker_pkg";\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if [ -z "$(echo $LANG | grep -e 'en_US.UTF-8' -e 'C.UTF-8')" ]; then \
  if [ -n "$(locale -a | grep 'en_US.utf8')" ]; then export LC_ALL=en_US.UTF-8;\
  else export LC_ALL=C.UTF-8; fi;\
fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo;\
  if ! command -v sudo > /dev/null 2>&1; then sudo; exit 1; fi;\
fi;\
if ! command -v fuser > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst psmisc;\
  if ! command -v fuser > /dev/null 2>&1; then fuser; exit 1; fi;\
fi;\
if ! command -v lsof > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst lsof;\
  if ! command -v lsof > /dev/null 2>&1; then lsof; exit 1; fi;\
fi;\
if ! command -v docker > /dev/null 2>&1; then sudo $pm $check_pkgs;\
  if [ -n "$($pm $what_pkg $docker_pkg | grep 'moby-engine')" ]; then echo "Docker is not supported"; docker; exit 1;\
  else sudo $pm $silent_inst $docker_pkg;\
    if ! command -v docker > /dev/null 2>&1; then docker; exit 1;\
    elif [ -n "$(docker --version | grep 'podman')" ]; then check_srv="podman.socket"; sudo touch /etc/containers/nodocker;\
    else check_srv="docker"; fi;\
  sleep 5; sudo systemctl enable --now $check_srv; sleep 5;\
  fi;\
fi;\
if [ -n "$(docker --version | grep 'moby-engine')" ]; then echo "Docker is not supported"; echo "command not found"; exit 1;\
elif [ -n "$(docker --version | grep 'podman')" ]; then check_srv="podman.socket"; docker_pkg="podman-docker";\
  if [ -n "$(docker --version 2>&1 | grep '/etc/containers/nodocker')" ]; then sudo touch /etc/containers/nodocker; fi;\
  sudo sed -i 's/short-name-mode = "enforcing"/short-name-mode = "permissive"/g' /etc/containers/registries.conf;\
else check_srv="docker"; fi;\
if [ "$(systemctl is-active $check_srv)" != "active" ]; then \
  sudo $pm $check_pkgs; sudo $pm $silent_inst $docker_pkg;\
  sleep 5; sudo systemctl start $check_srv; sleep 5;\
  if [ "$(systemctl is-active $check_srv)" != "active" ]; then echo "Failed to status docker"; echo "command not found"; exit 1; fi;\
fi;\
docker --version
