pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; if [[ ! -f "/usr/bin/sudo" ]]; then $pm update -y -q; $pm install -y -q sudo; fi
pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; sudo $pm update -y -q
pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then pm=$pm_apt; else pm=$pm_yum; fi; sudo $pm install -y -q curl
pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum"; if [[ -f "$pm_apt" ]]; then sudo export DEBIAN_FRONTEND=noninteractive; sudo $pm_apt install -y -q docker.io; else sudo $pm_yum install -y -q docker; fi
sudo systemctl start docker

