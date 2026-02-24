package main

import (
	"log"
	"os"

	"golang.org/x/crypto/ssh"
)

func main() {
	user := "root"
	pass := "1Q8ntQZYV40E"
	host := "138.124.101.69:22"

	config := &ssh.ClientConfig{
		User: user,
		Auth: []ssh.AuthMethod{
			ssh.Password(pass),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	client, err := ssh.Dial("tcp", host, config)
	if err != nil {
		log.Fatalf("SSH fail: %v", err)
	}
	defer client.Close()

	session, _ := client.NewSession()
	out, _ := session.Output("docker exec amnezia-awg2 cat /opt/amnezia/awg/awg0.conf")
	os.WriteFile("server_config.txt", out, 0644)

	session2, _ := client.NewSession()
	out2, _ := session2.Output("docker exec amnezia-awg2 iptables -t nat -L POSTROUTING -n -v")
	os.WriteFile("server_nat.txt", out2, 0644)

	session3, _ := client.NewSession()
	out3, _ := session3.Output("docker exec amnezia-awg2 iptables -L FORWARD -n -v")
	os.WriteFile("server_forward.txt", out3, 0644)
}
