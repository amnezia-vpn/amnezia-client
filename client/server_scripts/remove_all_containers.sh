sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker stop;\
sudo docker ps -a | grep amnezia | awk '{print $1}' | xargs sudo docker rm -fv;\
sudo docker images -a --format table | grep amnezia | awk '{print $3, $1 ":" $2}' | xargs sudo docker rmi;\
sudo docker volume ls | grep amnezia | awk '{print $2}' | xargs sudo docker volume rm -f;\
sudo docker network ls | grep amnezia-dns-net | awk '{print $1}' | xargs sudo docker network rm;\
sudo rm -frd /opt/amnezia
