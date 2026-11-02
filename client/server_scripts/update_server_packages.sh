if pm=$(command -v apt-get 2>/dev/null); then pm_env="DEBIAN_FRONTEND=noninteractive NEEDRESTART_MODE=l"; update_pkgs="-yq update"; upgrade_pkgs="-yq -o Dpkg::Options::=--force-confdef -o Dpkg::Options::=--force-confold dist-upgrade"; dist="debian";\
elif pm=$(command -v dnf 2>/dev/null); then update_pkgs="-yq makecache"; upgrade_pkgs="-yq upgrade"; dist="fedora";\
elif pm=$(command -v yum 2>/dev/null); then update_pkgs="-y -q makecache"; upgrade_pkgs="-y -q update"; dist="centos";\
elif pm=$(command -v zypper 2>/dev/null); then update_pkgs="-nq refresh"; upgrade_pkgs="-nq update -l"; dist="suse";\
elif pm=$(command -v pacman 2>/dev/null); then upgrade_pkgs="-Syu --noconfirm --noprogressbar --quiet"; dist="archlinux";\
else echo "Packet manager not found"; echo "Packages update failed"; exit 1;\
fi;\
echo "Dist: $dist, Packet manager: $pm";\
echo $LANG | grep -qE '^(en_US.UTF-8|C.UTF-8|C)$' || export LC_ALL=C;\
run_pm="sudo -n env $pm_env $pm";\
rc=0;\
if [ -n "$update_pkgs" ]; then $run_pm $update_pkgs; rc=$?; echo "Package index update exit code: $rc"; fi;\
if [ $rc -eq 0 ]; then \
  $run_pm $upgrade_pkgs; rc=$?;\
  if [ "$dist" = "suse" ] && [ $rc -eq 103 ]; then $run_pm $upgrade_pkgs; rc=$?; fi;\
  if [ "$dist" = "suse" ] && [ $rc -eq 102 ]; then rc=0; fi;\
  echo "Package upgrade exit code: $rc";\
fi;\
if [ $rc -eq 0 ]; then echo "Packages update finished"; else echo "Packages update failed"; fi
