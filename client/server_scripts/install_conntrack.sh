if command -v conntrack > /dev/null 2>&1; then echo "conntrack already installed"; exit 0; fi;\
if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); silent_inst="-yq install --install-recommends"; check_pkgs="-yq update"; conntrack_pkg="conntrack"; dist="debian";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); silent_inst="-yq install"; check_pkgs="-yq check-update"; conntrack_pkg="conntrack-tools"; dist="fedora";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); silent_inst="-y -q install"; check_pkgs="-y -q check-update"; conntrack_pkg="conntrack-tools"; dist="centos";\
elif which zypper > /dev/null 2>&1; then pm=$(which zypper); silent_inst="-nq install"; check_pkgs="-nq refresh"; conntrack_pkg="conntrack-tools"; dist="opensuse";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); silent_inst="-S --noconfirm --noprogressbar --quiet"; check_pkgs="-Sup"; conntrack_pkg="conntrack-tools"; dist="archlinux";\
else echo "Packet manager not found"; exit 0; fi;\
if [ "$dist" = "debian" ]; then export DEBIAN_FRONTEND=noninteractive; fi;\
sudo $pm $check_pkgs; sudo $pm $silent_inst $conntrack_pkg;\
command -v conntrack > /dev/null 2>&1 && echo "conntrack installed" || echo "conntrack install failed"
