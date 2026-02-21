package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net"

	"github.com/google/uuid"
	"golang.org/x/crypto/ssh"
)

// XrayServerSettings holds server-specific VLESS+Reality parameters.
type XrayServerSettings struct {
	Port        int    `json:"port"`
	Flow        string `json:"flow"`
	Security    string `json:"security"`    // "reality"
	SNI         string `json:"sni"`         // e.g. "google.com"
	Fingerprint string `json:"fingerprint"` // e.g. "chrome"
	PublicKey   string `json:"public_key"`
	ShortID     string `json:"short_id"`
	SpiderX     string `json:"spider_x"`
	Encryption  string `json:"encryption"`
	PQV         string `json:"pqv"`
}

// AmneziaProvider connects to an AmneziaVPN server via SSH, manipulates the Xray config,
// and manages docker container to provision clients.
type AmneziaProvider struct {
	serverHost string
	sshUser    string
	sshPass    string
	settings   XrayServerSettings // Used for generating the vless:// URI
}

// NewAmneziaProvider creates a new Amnezia provider.
func NewAmneziaProvider(serverHost, sshUser, sshPass, settingsJSON string) *AmneziaProvider {
	var settings XrayServerSettings
	if err := json.Unmarshal([]byte(settingsJSON), &settings); err != nil {
		log.Printf("Warning: failed to parse amnezia xray settings: %v", err)
		settings = XrayServerSettings{
			Port:        443,
			Flow:        "xtls-rprx-vision",
			Security:    "reality",
			SNI:         "google.com",
			Fingerprint: "chrome",
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

// getAmneziaConfig fetches and parses the Amnezia Xray config file via SSH.
func (p *AmneziaProvider) getAmneziaConfig(client *ssh.Client) (map[string]interface{}, error) {
	const configPath = "/opt/amnezia/xray/server.json"
	out, err := p.executeCommand(client, "docker exec amnezia-xray cat "+configPath)
	if err != nil {
		return nil, fmt.Errorf("failed to read Xray config: %w", err)
	}

	var config map[string]interface{}
	if err := json.Unmarshal([]byte(out), &config); err != nil {
		return nil, fmt.Errorf("failed to parse Xray config JSON: %w", err)
	}

	return config, nil
}

// writeAmneziaConfig marshals the config and writes it back to the server, then restarts the container.
func (p *AmneziaProvider) writeAmneziaConfig(client *ssh.Client, config map[string]interface{}) error {
	configBytes, err := json.MarshalIndent(config, "", "  ")
	if err != nil {
		return fmt.Errorf("failed to marshal modified config: %w", err)
	}

	const configPath = "/opt/amnezia/xray/server.json"

	// Pass JSON safely to docker container via stdin and heredoc
	cmd := fmt.Sprintf("cat << 'EOF' | docker exec -i amnezia-xray sh -c 'cat > %s'\n%s\nEOF\ndocker restart amnezia-xray", configPath, string(configBytes))

	_, err = p.executeCommand(client, cmd)
	if err != nil {
		return fmt.Errorf("failed to update config and restart container: %w", err)
	}

	return nil
}

// findInbound finds the first inbound suitable for adding clients (usually VLESS).
func findInbound(config map[string]interface{}) (map[string]interface{}, error) {
	inbounds, ok := config["inbounds"].([]interface{})
	if !ok || len(inbounds) == 0 {
		return nil, fmt.Errorf("no inbounds array found in config")
	}

	for _, ib := range inbounds {
		inbound, ok := ib.(map[string]interface{})
		if !ok {
			continue
		}
		// Looking for protocol vless
		protocol, _ := inbound["protocol"].(string)
		if protocol == "vless" {
			settings, ok := inbound["settings"].(map[string]interface{})
			if !ok {
				settings = map[string]interface{}{}
				inbound["settings"] = settings
			}
			return settings, nil
		}
	}

	return nil, fmt.Errorf("no vless inbound found")
}

// CreateKey connects via SSH, injects a new client to the config.json, and restarts Xray.
func (p *AmneziaProvider) CreateKey(userID string) (string, string, error) {
	email := fmt.Sprintf("user-%s", userID)

	client, err := p.connectSSH()
	if err != nil {
		return "", "", fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	config, err := p.getAmneziaConfig(client)
	if err != nil {
		return "", "", err
	}

	settings, err := findInbound(config)
	if err != nil {
		return "", "", err
	}

	clients, _ := settings["clients"].([]interface{})

	// Check if exists
	for _, c := range clients {
		clientMap, ok := c.(map[string]interface{})
		if ok {
			if e, _ := clientMap["email"].(string); e == email {
				id, _ := clientMap["id"].(string)
				log.Printf("User %s already exists on Amnezia, reusing key", userID)
				return id, p.buildAmneziaURI(config, id), nil
			}
		}
	}

	newID := uuid.New().String()
	newClient := map[string]interface{}{
		"id":    newID,
		"flow":  p.settings.Flow,
		"email": email,
	}

	settings["clients"] = append(clients, newClient)

	// Write changes
	if err := p.writeAmneziaConfig(client, config); err != nil {
		return "", "", err
	}

	return newID, p.buildAmneziaURI(config, newID), nil
}

// GetKeys fetches all current clients from the remote config.
func (p *AmneziaProvider) GetKeys() ([]VPNKey, error) {
	client, err := p.connectSSH()
	if err != nil {
		return nil, fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	config, err := p.getAmneziaConfig(client)
	if err != nil {
		return nil, err
	}

	settings, err := findInbound(config)
	if err != nil {
		return nil, err
	}

	var keys []VPNKey
	clients, _ := settings["clients"].([]interface{})
	for _, c := range clients {
		clientMap, ok := c.(map[string]interface{})
		if ok {
			id, _ := clientMap["id"].(string)
			email, _ := clientMap["email"].(string)
			if id != "" {
				keys = append(keys, VPNKey{
					ID:        id,
					Name:      email,
					AccessURL: p.buildAmneziaURI(config, id),
				})
			}
		}
	}

	return keys, nil
}

// DeleteKey removes the client with the matching id and restarts the Xray container.
func (p *AmneziaProvider) DeleteKey(keyID string) error {
	client, err := p.connectSSH()
	if err != nil {
		return fmt.Errorf("SSH connection failed: %w", err)
	}
	defer client.Close()

	config, err := p.getAmneziaConfig(client)
	if err != nil {
		return err
	}

	settings, err := findInbound(config)
	if err != nil {
		return err
	}

	clients, _ := settings["clients"].([]interface{})
	var newClients []interface{}
	found := false

	for _, c := range clients {
		clientMap, ok := c.(map[string]interface{})
		if ok {
			id, _ := clientMap["id"].(string)
			if id == keyID {
				found = true
				continue // skip this one
			}
		}
		newClients = append(newClients, c)
	}

	if !found {
		return nil // Nothing to delete
	}

	settings["clients"] = newClients

	// Write changes
	return p.writeAmneziaConfig(client, config)
}

func (p *AmneziaProvider) SetName(keyID string, name string) error {
	return nil // No-op, email is immutable mostly and tracked by DB
}

func (p *AmneziaProvider) buildAmneziaURI(fullConfig map[string]interface{}, uuid string) string {
	// Deep copy the config to avoid mutating the original
	configBytes, _ := json.Marshal(fullConfig)
	var clientConfig map[string]interface{}
	json.Unmarshal(configBytes, &clientConfig)

	// Keep only this user in the inbounds
	if settings, err := findInbound(clientConfig); err == nil {
		if clients, ok := settings["clients"].([]interface{}); ok {
			for _, c := range clients {
				if cmap, ok := c.(map[string]interface{}); ok {
					if id, _ := cmap["id"].(string); id == uuid {
						settings["clients"] = []interface{}{cmap}
						break
					}
				}
			}
		}
	}

	// For the client, the inbound listen address is usually 127.0.0.1, but Amnezia's client
	// parser looks for the server IP in outbounds. The server config we just read is for the server itself.
	// We need to convert this *server* config into a *client* config.
	// Actually, the Amnezia client exporter expects an "amnezia-xray" container payload or a standard client config.
	// Let's generate a raw xray client config.

	clientPort := p.settings.Port
	if clientPort == 0 {
		clientPort = 443
	}

	clientPayload := map[string]interface{}{
		"log": map[string]interface{}{
			"loglevel": "warning",
		},
		"inbounds": []map[string]interface{}{
			{
				"tag":      "socks-in",
				"port":     10808,
				"listen":   "127.0.0.1",
				"protocol": "socks",
				"settings": map[string]interface{}{"auth": "noauth", "udp": true},
			},
		},
		"outbounds": []map[string]interface{}{
			{
				"protocol": "vless",
				"settings": map[string]interface{}{
					"vnext": []map[string]interface{}{
						{
							"address": p.serverHost,
							"port":    clientPort,
							"users": []map[string]interface{}{
								{
									"id":         uuid,
									"encryption": p.settings.Encryption,
									"flow":       p.settings.Flow,
								},
							},
						},
					},
				},
				"streamSettings": map[string]interface{}{
					"network":  "tcp",
					"security": "reality",
					"realitySettings": map[string]interface{}{
						"serverName":  p.settings.SNI,
						"fingerprint": p.settings.Fingerprint,
						"publicKey":   p.settings.PublicKey,
						"shortId":     p.settings.ShortID,
						"spiderX":     p.settings.SpiderX,
					},
				},
			},
		},
	}

	// Wrap it in amnezia format so the Desktop client's ParseAmneziaConfig is happy
	wrap := map[string]interface{}{
		"containers": []map[string]interface{}{
			{
				"container":     "amnezia-xray",
				"client_config": clientPayload,
			},
		},
	}

	data, _ := json.Marshal(wrap)
	b64 := base64.StdEncoding.EncodeToString(data)
	return "amnezia://" + b64
}
