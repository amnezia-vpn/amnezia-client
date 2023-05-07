# Run container
sudo docker network create meet.jitsi && docker run \
 -p 443:443 \
 -p 80:80 \
 -p 5222:5222 \
 --hostname xmpp.jitsi.meet \
 --network meet.jitsi \
 --name $CONTAINER_NAME $CONTAINER_NAME
