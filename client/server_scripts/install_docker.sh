pm_apt="/usr/bin/apt-get"; pm_yum="/usr/bin/yum";\
if [[ -f "$pm_apt" ]]; then pm=$pm_apt; docker_pkg="docker.io"; else pm=$pm_yum; docker_pkg="docker"; fi;\
if [[ ! -f "/usr/bin/sudo" ]]; then $pm update -y -q; $pm install -y -q sudo; fi;\
docker_service=$(systemctl list-units --full -all | grep docker.service);\
if [[ -f "$pm_apt" ]]; then export DEBIAN_FRONTEND=noninteractive; fi;\
if [[ -z "$docker_service" ]]; then sudo $pm update -y -q; sudo $pm install -y -q curl $docker_pkg; fi;\
sudo systemctl start docker
