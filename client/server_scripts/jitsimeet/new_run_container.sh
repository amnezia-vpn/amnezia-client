# Run container
sudo docker network create meet.jitsi && \
sudo docker run -d \
 -p 8443:443 \
 -p 8000:80 \
  --hostname xmpp.meet.jitsi \
 --network meet.jitsi \
 --name amnezia-jitsi amnezia-jitsi && \

sudo docker run -d \
 -e JVB_AUTH_USER=admin \
 -e JVB_AUTH_PASSWORD=admin \
 -e JICOFO_AUTH_PASSWORD=admin \
 --network meet.jitsi \
 --hostname xmpp.meet.jitsi \
 --name amnezia-prosody jitsi/prosody:unstable && \

sudo docker run -d \
 -e JICOFO_AUTH_PASSWORD=admin \
 --network meet.jitsi \
  --hostname xmpp.meet.jitsi \
  --name amnezia-jicofo jitsi/jicofo:unstable && \

sudo docker run -d \
 -e JVB_AUTH_USER=admin \
 -e JVB_AUTH_PASSWORD=admin \
 --network meet.jitsi \
 --hostname xmpp.meet.jitsi \
 --name amnezia-jvb jitsi/jvb:unstable && \

sudo docker network connect meet.jitsi amnezia-jitsi && \
sudo docker network connect meet.jitsi amnezia-prosody && \
sudo docker network connect meet.jitsi amnezia-jicofo && \
sudo docker network connect meet.jitsi amnezia-jvb
