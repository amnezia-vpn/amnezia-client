package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"strings"

	"github.com/google/uuid"
	"golang.org/x/crypto/ssh"
)

// XrayServerSettings holds server-specific VLESS+Reality parameters.
type XrayServerSettings struct {
	Port int `json:"port"`
	// Additional Xray settings ignored for AWG
}

// VPNKey is defined in provider.go

// AmneziaProvider implementations SSH provisioning directly to Amnezia VPN nodes.
type AmneziaProvider struct {
	serverHost string
	sshUser    string
	sshPass    string
	settings   XrayServerSettings // Used for generating the vless:// URI
}

func NewAmneziaProvider(serverHost, sshUser, sshPass, settingsJSON string) *AmneziaProvider {
	var settings XrayServerSettings
	if err := json.Unmarshal([]byte(settingsJSON), &settings); err != nil {
		log.Printf("Warning: failed to parse amnezia xray settings: %v", err)
		settings = XrayServerSettings{
			Port: 443,
		}
	}

	return &AmneziaProvider{
		serverHost: serverHost,
		sshUser:    sshUser,
		sshPass:    sshPass,
		settings:   settings,
	}
}

// connectSSH establishes an SSH connection.
func (p *AmneziaProvider) connectSSH() (*ssh.Client, error) {
	config := &ssh.ClientConfig{
		User: p.sshUser,
		Auth: []ssh.AuthMethod{
			ssh.Password(p.sshPass),
		},
		HostKeyCallback: ssh.InsecureIgnoreHostKey(),
	}

	addr := net.JoinHostPort(p.serverHost, "22")
	return ssh.Dial("tcp", addr, config)
}

// executeCommand runs a command over SSH and returns its output.
func (p *AmneziaProvider) executeCommand(client *ssh.Client, cmd string) (string, error) {
	session, err := client.NewSession()
	if err != nil {
		return "", err
	}
	defer session.Close()

	var stdoutBuf bytes.Buffer
	var stderrBuf bytes.Buffer
	session.Stdout = &stdoutBuf
	session.Stderr = &stderrBuf

	err = session.Run(cmd)
	if err != nil {
		return "", fmt.Errorf("command execution failed: %w, stderr: %s", err, stderrBuf.String())
	}

	return stdoutBuf.String(), nil
}

// getAmneziaWGConfig fetches the AmneziaWG config file via SSH.
func (p *AmneziaProvider) getAmneziaWGConfig(client *ssh.Client) (string, error) {
	const configPath = "/opt/amnezia/awg/awg0.conf"
	out, err := p.executeCommand(client, "docker exec amnezia-awg2 cat "+configPath)
	if err != nil {
		return "", fmt.Errorf("failed to read AWG config: %w", err)
	}
	return out, nil
}

// writeAmneziaWGConfig writes it back to the server and restarts the container.
func (p *AmneziaProvider) writeAmneziaWGConfig(client *ssh.Client, config string) error {
	const configPath = "/opt/amnezia/awg/awg0.conf"

	// Pass config safely to docker container
	cmd := fmt.Sprintf("cat << 'EOF' | docker exec -i amnezia-awg2 sh -c 'cat > %s'\n%s\nEOF\ndocker exec amnezia-awg2 awg-quick down %s; docker exec amnezia-awg2 awg-quick up %s", configPath, config, configPath, configPath)

	_, err := p.executeCommand(client, cmd)
	if err != nil {
		return fmt.Errorf("failed to update config and restart awg: %w", err)
	}

	return nil
}

// CreateKey connects via SSH, injects a new PEER to wg0.conf.
func (p *AmneziaProvider) CreateKey(userID string) (string, string, error) {
	client, err := p.connectSSH()
	if err != nil {
		return "", "", fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	// 1. Generate new keys using standard `wg / awg` command on the server
	privKey, err := p.executeCommand(client, "docker exec amnezia-awg2 awg genkey")
	if err != nil {
		return "", "", fmt.Errorf("failed to generate privkey: %w", err)
	}
	privKey = strings.TrimSpace(privKey)

	pubKey, err := p.executeCommand(client, fmt.Sprintf("echo '%s' | docker exec -i amnezia-awg2 awg pubkey", privKey))
	if err != nil {
		return "", "", fmt.Errorf("failed to generate pubkey: %w", err)
	}
	pubKey = strings.TrimSpace(pubKey)

	psk, err := p.executeCommand(client, "docker exec amnezia-awg2 awg genpsk")
	if err != nil {
		return "", "", fmt.Errorf("failed to generate psk: %w", err)
	}
	psk = strings.TrimSpace(psk)

	// 2. Fetch existing config to find the next available IP
	config, err := p.getAmneziaWGConfig(client)
	if err != nil {
		return "", "", err
	}

	// Simple IP allocation (find max IP 10.8.1.X and add 1)
	nextIP := 2
	lines := strings.Split(config, "\n")
	for _, line := range lines {
		if strings.HasPrefix(strings.TrimSpace(line), "AllowedIPs") {
			parts := strings.Split(line, "=")
			if len(parts) == 2 {
				ip := strings.TrimSpace(strings.Split(parts[1], "/")[0])
				var a, b, c, d int
				fmt.Sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d)
				if d >= nextIP {
					nextIP = d + 1
				}
			}
		}
	}
	if nextIP > 254 {
		return "", "", fmt.Errorf("IP pool exhausted")
	}

	clientIP := fmt.Sprintf("10.8.1.%d", nextIP)
	uuidID := uuid.New().String()

	// 3. Extract the server's public key BEFORE we restart the awg0 interface!
	serverPubKey, err := p.executeCommand(client, "docker exec amnezia-awg2 awg show awg0 public-key")
	if err != nil || strings.TrimSpace(serverPubKey) == "" {
		log.Printf("[AWG] Warning: fallback to parsing privkey due to awg show error: %v", err)
		for _, line := range lines {
			if strings.HasPrefix(strings.TrimSpace(line), "PrivateKey") {
				parts := strings.Split(line, "=")
				if len(parts) == 2 {
					serverPriv := strings.TrimSpace(parts[1])
					sp, _ := p.executeCommand(client, fmt.Sprintf("echo '%s' | docker exec -i amnezia-awg2 awg pubkey", serverPriv))
					serverPubKey = strings.TrimSpace(sp)
					break
				}
			}
		}
	}
	serverPubKey = strings.TrimSpace(serverPubKey)

	// 4. Append to wg0.conf
	newPeer := fmt.Sprintf("\n# User: %s\n# ID: %s\n[Peer]\nPublicKey = %s\nPresharedKey = %s\nAllowedIPs = %s/32\n", userID, uuidID, pubKey, psk, clientIP)
	config += newPeer

	if err := p.writeAmneziaWGConfig(client, config); err != nil {
		return "", "", err
	}

	// 5. Generate client URI
	clientConf := fmt.Sprintf(`[Interface]
Address = %s/32
DNS = 1.1.1.1, 8.8.8.8
PrivateKey = %s
Jc = 4
Jmin = 40
Jmax = 70
S1 = 0
S2 = 0
H1 = 1
H2 = 2
H3 = 3
H4 = 4

[Peer]
PublicKey = %s
PresharedKey = %s
AllowedIPs = 0.0.0.0/0, ::/0
Endpoint = %s:%d
PersistentKeepalive = 25
`, clientIP, privKey, serverPubKey, psk, p.serverHost, p.settings.Port)

	uri := "amneziawg://" + base64.StdEncoding.EncodeToString([]byte(clientConf))
	return uuidID, uri, nil
}

// GetKeys fetches all current clients from wg0.conf.
func (p *AmneziaProvider) GetKeys() ([]VPNKey, error) {
	client, err := p.connectSSH()
	if err != nil {
		return nil, fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	config, err := p.getAmneziaWGConfig(client)
	if err != nil {
		return nil, err
	}

	var keys []VPNKey

	// Scan wg0.conf for "# ID: <uuid>"
	lines := strings.Split(config, "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "# ID:") {
			parts := strings.Split(line, ":")
			if len(parts) == 2 {
				id := strings.TrimSpace(parts[1])

				// We can't trivially reconstruct the exact client URI here because we don't store
				// the client's PrivateKey on the server (only the public key in wg0.conf).
				// We rely on the App DB `access_keys.access_url` to persist the original URI.
				// However, GetKeys is only used by handlers.go to avoid re-creating a key
				// if `access_keys` is wiped.
				// If access_keys is wiped, the user's private key is permanently lost.
				// For the sake of matching, we'll return a placeholder URL.
				keys = append(keys, VPNKey{
					ID:        id,
					Name:      id,
					AccessURL: "lost-private-key",
				})
			}
		}
	}

	return keys, nil
}

// DeleteKey removes the peer from wg0.conf and restarts AWG.
func (p *AmneziaProvider) DeleteKey(keyID string) error {
	client, err := p.connectSSH()
	if err != nil {
		return fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	config, err := p.getAmneziaWGConfig(client)
	if err != nil {
		return err
	}

	lines := strings.Split(config, "\n")
	var newLines []string
	skip := false

	for i := 0; i < len(lines); i++ {
		line := lines[i]

		// Start of a peer definition
		if strings.HasPrefix(line, "# ID: "+keyID) {
			skip = true // Skip this line and subsequent lines until the next peer or EOF
			// also remove the previous line if it was `# User: ...`
			if len(newLines) > 0 && strings.HasPrefix(newLines[len(newLines)-1], "# User:") {
				newLines = newLines[:len(newLines)-1]
			}
			continue
		}

		if skip {
			if strings.HasPrefix(line, "# User:") || (strings.HasPrefix(line, "[") && !strings.Contains(line, "[Peer]")) {
				skip = false // Hit the next block, stop skipping
			} else {
				continue
			}
		}

		newLines = append(newLines, line)
	}

	newConfig := strings.Join(newLines, "\n")

	return p.writeAmneziaWGConfig(client, newConfig)
}

func (p *AmneziaProvider) SetName(keyID string, name string) error {
	return nil // No-op
}
