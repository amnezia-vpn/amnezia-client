if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install"; check_pkgs="-yq update"; docker_pkg="docker.io"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; check_pkgs="-yq check-update"; docker_pkg="docker"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; check_pkgs="-y -q check-update"; docker_pkg="docker"; dist="centos";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="--noconfirm -S"; check_pkgs="--noconfirm -Sy"; docker_pkg="docker"; dist="archlinux";\
else echo "Packet manager not found"; exit 1; fi;\
echo "Dist: $dist, Packet manager: $pm, Install command: $silent_inst, Check pkgs command: $check_pkgs, Docker pkg: $docker_pkg";\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if ! command -v sudo > /dev/null 2>&1; then $pm $check_pkgs; $pm $silent_inst sudo; fi;\
if ! command -v fuser > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst psmisc; fi;\
if ! command -v lsof > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst lsof; fi;\
if ! command -v docker > /dev/null 2>&1; then sudo $pm $check_pkgs; sudo $pm $silent_inst $docker_pkg;\
  if [ "$dist" = "fedora" ] || [ "$dist" = "centos" ] || [ "$dist" = "debian" ]; then sudo systemctl enable docker && sudo systemctl start docker; fi;\
fi;\
if [ "$dist" = "debian" ]; then \
  docker_service=$(systemctl list-units --full --all | grep docker.service | grep -v inactive | grep -v dead | grep -v failed);\
  if [ -z "$docker_service" ]; then sudo $pm $check_pkgs; sudo $pm $silent_inst curl $docker_pkg; fi;\
  sleep 3 && sudo systemctl start docker && sleep 3;\
fi;\
if ! command -v sudo > /dev/null 2>&1; then echo "Failed to install Docker"; exit 1; fi;\
docker --version