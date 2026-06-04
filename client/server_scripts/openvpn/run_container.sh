# Run container
sudo docker run -d \
--privileged \
--log-driver none \
--restart always \
--cap-add=NET_ADMIN \
-p $OPENVPN_PORT:$OPENVPN_PORT/$OPENVPN_TRANSPORT_PROTO \
--name $CONTAINER_NAME $CONTAINER_NAME

# Wait until the container is actually running before the network/exec steps.
# DH generation was removed, so this phase now finishes in ~2s instead of up to
# minutes; without this guard a slow Docker daemon (or a leftover container from a
# previous deploy) could let later steps race ahead and fail with "No such
# container". Single-line on purpose (script runs as one batched shell). ~15s cap.
amn_i=0; while [ "$(sudo docker inspect -f '{{.State.Running}}' $CONTAINER_NAME 2>/dev/null)" != "true" ] && [ $amn_i -lt 30 ]; do sleep 0.5; amn_i=$((amn_i+1)); done

sudo docker network connect amnezia-dns-net $CONTAINER_NAME

# Create tun device if not exist
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /dev/net; if [ ! -c /dev/net/tun ]; then mknod /dev/net/tun c 10 200; fi'

# Prevent to route packets outside of the container in case if server behind of the NAT
sudo docker exec -i $CONTAINER_NAME sh -c "ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up"

# OpenVPN config
sudo docker exec -i $CONTAINER_NAME bash -c 'mkdir -p /opt/amnezia/openvpn/clients; \
cd /opt/amnezia/openvpn && easyrsa init-pki; \
cd /opt/amnezia/openvpn && easyrsa build-ca nopass << EOF yes EOF && easyrsa gen-req AmneziaReq nopass << EOF2 yes EOF2;\
cd /opt/amnezia/openvpn && easyrsa sign-req server AmneziaReq << EOF3 yes EOF3;\
cd /opt/amnezia/openvpn && openvpn --genkey --secret ta.key << EOF4;\
cd /opt/amnezia/openvpn && cp pki/ca.crt pki/issued/AmneziaReq.crt pki/private/AmneziaReq.key /opt/amnezia/openvpn;\
cd /opt/amnezia/openvpn && easyrsa gen-crl;\
cd /opt/amnezia/openvpn && cp pki/crl.pem /opt/amnezia/openvpn/crl.pem'
