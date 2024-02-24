sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker stop;\
sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker rm -fv;\
sudo docker images -a | grep amnezia | awk '{print $3}' | xargs sudo docker rmi;\
sudo docker network ls | grep amnezia-dns-net | awk '{print $1}' | xargs sudo docker network rm;\
sudo rm -frd /opt/amnezia
