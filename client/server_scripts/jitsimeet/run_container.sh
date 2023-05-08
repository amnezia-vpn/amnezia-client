# Run container
sudo docker network create meet.jitsi && sudo docker run \
 -p 443:443 \
 -p 80:80 \
 -p 5222:5222 \
 --hostname xmpp.meet.jitsi \
 --network meet.jitsi \
 --name $CONTAINER_NAME $CONTAINER_NAME
