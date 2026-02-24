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
	out, _ := session.Output("docker inspect amnezia-awg2 --format='{{json .NetworkSettings.Ports}}'")
	os.WriteFile("ports.txt", out, 0644)

	session2, _ := client.NewSession()
	out2, _ := session2.Output("docker logs amnezia-awg2 | tail -n 20")
	os.WriteFile("awg_logs.txt", out2, 0644)
}
