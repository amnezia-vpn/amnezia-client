docker container stop $(docker ps -a -q  --filter ancestor="alekslitvinenk/openvpn")
docker container kill $(docker ps -a -q  --filter ancestor="alekslitvinenk/openvpn")