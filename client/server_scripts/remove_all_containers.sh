sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker stop;\
if [ -n "$(sudo docker --version 2>/dev/null | grep podman)" ]; then \
  sudo systemctl list-units | grep amnezia | awk '{print $1}' | xargs sudo systemctl disable --now;\
  sudo systemctl daemon-reload; sudo systemctl reset-failed;\
  sudo rm -f /etc/systemd/system/container-amnezia-*.service;\
  sudo systemctl daemon-reload;\
  sudo sed -i '/^  # Amnezia start/,/^  # Amnezia finish$/d' /var/cache/containers/short-name-aliases.conf;\
fi;\
sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker rm -fv;\
sudo docker images -a | grep amnezia | awk '{print $3}' | xargs sudo docker rmi;\
sudo docker network ls | grep amnezia-dns-net | awk '{print $1}' | xargs sudo docker network rm;\
sudo rm -frd /opt/amnezia
