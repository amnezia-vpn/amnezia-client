if which apt-get > /dev/null 2>&1; then pm=$(which apt-get); opt="--version";\
elif which dnf > /dev/null 2>&1; then pm=$(which dnf); opt="--version";\
elif which yum > /dev/null 2>&1; then pm=$(which yum); opt="--version";\
elif which zypper > /dev/null 2>&1; then pm=$(which zypper); opt="--version";\
elif which pacman > /dev/null 2>&1; then pm=$(which pacman); opt="--version";\
else pm="uname"; opt="-a";\
fi;\
CUR_USER=$(whoami 2>/dev/null || echo $HOME | sed 's/.*\///');\
echo $LANG | grep -qE '^(en_US.UTF-8|C.UTF-8|C)$' || export LC_ALL=C;\
sudo -K;\
cd ~;\
if [ "$CUR_USER" = "root" ] || ( groups "$CUR_USER" | grep -E '\<(sudo|wheel)\>' ); then \
  sudo -nu $CUR_USER $pm $opt > /dev/null; sudo -n $pm $opt > /dev/null;\
fi
