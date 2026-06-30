if pm=$(which apt-get 2>/dev/null || command -v apt-get 2>/dev/null); then opt="--version";\
elif pm=$(which dnf 2>/dev/null || command -v dnf 2>/dev/null); then opt="--version";\
elif pm=$(which yum 2>/dev/null || command -v yum 2>/dev/null); then opt="--version";\
elif pm=$(which zypper 2>/dev/null || command -v zypper 2>/dev/null); then opt="--version";\
elif pm=$(which pacman 2>/dev/null || command -v pacman 2>/dev/null); then opt="--version";\
else pm="uname"; opt="-a";\
fi;\
CUR_USER=$(whoami 2>/dev/null || echo $HOME | sed 's/.*\///');\
echo $LANG | grep -qE '^(en_US.UTF-8|C.UTF-8|C)$' || export LC_ALL=C;\
sudo -K;\
cd ~;\
if [ "$CUR_USER" = "root" ] || ( groups "$CUR_USER" | grep -E '\<(sudo|wheel)\>' ); then \
  sudo -nu $CUR_USER $pm $opt > /dev/null; sudo -n $pm $opt > /dev/null;\
fi
