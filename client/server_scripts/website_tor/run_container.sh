# Run container
sudo docker run -d -p 80:80 --restart always --name amnezia-wp-tor tutum/wordpress
sudo docker run -d --link amnezia-wp-tor --name amnezia-tor goldy/tor-hidden-service
