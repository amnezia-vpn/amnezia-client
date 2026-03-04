package handlers

import (
	"fmt"
	"net"
	"strconv"
	"strings"
	"time"
	"vpn-backend/internal/models"

	"golang.org/x/crypto/ssh"
	"gorm.io/gorm"
)

// sshExec подключается к серверу по SSH и выполняет команду
func sshExec(server *models.VPNServer, cmd string) (string, error) {
	sshHost := server.SSHHost
	if sshHost == "" {
		sshHost = server.Host
	}
	sshPort := server.SSHPort
	if sshPort == 0 {
		sshPort = 22
	}
	sshUser := server.SSHUser
	if sshUser == "" {
		sshUser = "root"
	}

	config := &ssh.ClientConfig{
		User: sshUser,
		Auth: []ssh.AuthMethod{
			ssh.Password(server.SSHPassword),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(), // TODO: хранить fingerprint
		Timeout:         10 * time.Second,
	}

	// Очищаем sshHost от возможного порта (если Host пришел как IP:Port)
	hostWithoutPort := sshHost
	if h, _, err := net.SplitHostPort(sshHost); err == nil {
		hostWithoutPort = h
	}

	addr := fmt.Sprintf("%s:%d", hostWithoutPort, sshPort)
	client, err := ssh.Dial("tcp", addr, config)
	if err != nil {
		return "", fmt.Errorf("SSH dial failed: %w", err)
	}
	defer client.Close()

	session, err := client.NewSession()
	if err != nil {
		return "", fmt.Errorf("SSH session failed: %w", err)
	}
	defer session.Close()

	out, err := session.CombinedOutput(cmd)
	return string(out), err
}

// addAWGPeer добавляет WireGuard/AWG peer на сервере через SSH
// Работает с amnezia-awg2 контейнером через docker exec
func addAWGPeer(server *models.VPNServer, clientPublicKey, presharedKey, allowedIP string) error {
	if server.SSHPassword == "" {
		// SSH не настроен — пропускаем, peer нужно добавить вручную
		return nil
	}

	container := server.AWGContainer
	if container == "" {
		container = "amnezia-awg2"
	}
	iface := server.AWGInterface
	if iface == "" {
		iface = "awg0"
	}
	confPath := fmt.Sprintf("/opt/amnezia/awg/%s.conf", iface)

	// 1. Добавляем peer в память (работает сразу)
	addCmd := fmt.Sprintf(
		`docker exec %s sh -c 'echo "%s" > /tmp/psk.txt && awg set %s peer %s preshared-key /tmp/psk.txt allowed-ips %s/32 && rm /tmp/psk.txt'`,
		container, presharedKey, iface, clientPublicKey, allowedIP,
	)
	if _, err := sshExec(server, addCmd); err != nil {
		return fmt.Errorf("failed to add peer: %w", err)
	}

	// 2. Сохраняем в конфиг файл для персистентности
	saveCmd := fmt.Sprintf(
		`docker exec %s sh -c 'cat >> %s << EOF

[Peer]
PublicKey = %s
PresharedKey = %s
AllowedIPs = %s/32
EOF'`,
		container, confPath, clientPublicKey, presharedKey, allowedIP,
	)
	if _, err := sshExec(server, saveCmd); err != nil {
		// Не критично — peer уже в памяти
		return fmt.Errorf("peer added but config save failed: %w", err)
	}

	return nil
}

// removeAWGPeer удаляет WireGuard/AWG peer с сервера через SSH
func removeAWGPeer(server *models.VPNServer, clientPublicKey string) error {
	if server.SSHPassword == "" {
		return nil
	}

	container := server.AWGContainer
	if container == "" {
		container = "amnezia-awg2"
	}
	iface := server.AWGInterface
	if iface == "" {
		iface = "awg0"
	}
	confPath := fmt.Sprintf("/opt/amnezia/awg/%s.conf", iface)

	// 1. Удалить peer из памяти
	removeCmd := fmt.Sprintf(
		`docker exec %s awg set %s peer %s remove`,
		container, iface, clientPublicKey,
	)
	if _, err := sshExec(server, removeCmd); err != nil {
		return fmt.Errorf("failed to remove peer: %w", err)
	}

	// 2. Удалить из конфиг-файла (удаляем блок [Peer] с этим ключом)
	cleanCmd := fmt.Sprintf(
		`docker exec %s sh -c 'python3 -c "
import re
with open(\"%s\") as f:
    content = f.read()
# Удаляем блок [Peer] с нашим PublicKey
pattern = r\"\n\[Peer\]\n(?:[^\[]*\n)*?PublicKey = %s\n(?:[^\[]*)*\"
content = re.sub(pattern, \"\", content)
with open(\"%s\", \"w\") as f:
    f.write(content)
"'`,
		container, confPath, clientPublicKey, confPath,
	)
	sshExec(server, cleanCmd) // не критично если не удалится из файла

	return nil
}

// fetchServerPublicKey получает публичный ключ сервера из WireGuard/AWG через SSH
func fetchServerPublicKey(server *models.VPNServer) (string, error) {
	if server.SSHPassword == "" {
		return "", fmt.Errorf("no SSH password provided, cannot fetch public key")
	}

	container := server.AWGContainer
	if container == "" {
		container = "amnezia-awg2"
	}
	iface := server.AWGInterface
	if iface == "" {
		iface = "awg0"
	}

	cmd := fmt.Sprintf(`docker exec %s awg show %s public-key`, container, iface)
	out, err := sshExec(server, cmd)
	if err != nil {
		// Попробуем без docker exec (если WG установлен на хосте)
		cmdHost := fmt.Sprintf(`awg show %s public-key`, iface)
		out, err = sshExec(server, cmdHost)
		if err != nil {
			return "", fmt.Errorf("failed to get public key via docker and host: %w", err)
		}
	}
	return strings.TrimSpace(out), nil
}

// fetchServerConfigHeaders получает параметры обфускации из конфига сервера (awg0.conf)
func fetchServerConfigHeaders(server *models.VPNServer) error {
	if server.SSHPassword == "" {
		return fmt.Errorf("no SSH password provided")
	}

	container := server.AWGContainer
	if container == "" {
		container = "amnezia-awg2"
	}
	iface := server.AWGInterface
	if iface == "" {
		iface = "awg0"
	}

	confPath := fmt.Sprintf("/opt/amnezia/awg/%s.conf", iface)
	cmd := fmt.Sprintf(`docker exec %s cat %s`, container, confPath)
	out, err := sshExec(server, cmd)
	if err != nil {
		// fallback to host
		cmdHost := fmt.Sprintf(`cat %s`, confPath)
		out, err = sshExec(server, cmdHost)
		if err != nil {
			return fmt.Errorf("failed to read %s: %w", confPath, err)
		}
	}

	lines := strings.Split(out, "\n")
	for _, line := range lines {
		line = strings.TrimSpace(line)
		if strings.HasPrefix(line, "#") || line == "" {
			continue
		}
		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}
		key := strings.TrimSpace(parts[0])
		val := strings.TrimSpace(parts[1])

		switch key {
		case "Jc":
			server.Jc = val
		case "Jmin":
			server.Jmin = val
		case "Jmax":
			server.Jmax = val
		case "S1":
			server.S1 = val
		case "S2":
			server.S2 = val
		case "S3":
			server.S3 = val
		case "S4":
			server.S4 = val
		case "H1":
			server.H1 = val
		case "H2":
			server.H2 = val
		case "H3":
			server.H3 = val
		case "H4":
			server.H4 = val
		case "ListenPort":
			if port, err := strconv.Atoi(val); err == nil {
				server.AWGPort = port
			}
		}
	}
	return nil
}

// SyncAllServers асинхронно опрашивает все активные серверы и обновляет их конфигурацию по SSH
// Полезно запускать при старте бэкенда для поддержания актуальности ключей и параметров обфускации
func SyncAllServers(db *gorm.DB) {
	var servers []models.VPNServer
	if err := db.Where("active = ?", true).Find(&servers).Error; err != nil {
		fmt.Printf("[WARN] SyncAllServers: failed to fetch servers: %v\n", err)
		return
	}

	for i := range servers {
		server := &servers[i]

		pubKey, err := fetchServerPublicKey(server)
		if err == nil && pubKey != "" {
			server.PublicKey = pubKey
		} else {
			fmt.Printf("[WARN] SyncAllServers: failed to fetch pubkey for %s: %v\n", server.Name, err)
			continue
		}

		if err := fetchServerConfigHeaders(server); err != nil {
			fmt.Printf("[WARN] SyncAllServers: failed to fetch headers for %s: %v\n", server.Name, err)
			continue
		}

		if err := db.Save(server).Error; err != nil {
			fmt.Printf("[WARN] SyncAllServers: failed to save server %s: %v\n", server.Name, err)
		} else {
			fmt.Printf("[INFO] SyncAllServers: successfully synced %s\n", server.Name)
		}
	}
}
