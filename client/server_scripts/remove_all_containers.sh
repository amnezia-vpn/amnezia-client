sudo docker ps | grep amnezia | awk '{print $1}' | xargs sudo docker stop
sudo docker ps | grep amnezia | awk '{print $1}' | xargs sudo docker rm
sudo docker images -a | grep amnezia | awk '{print $3}' | xargs sudo docker rmi
