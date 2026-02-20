package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/url"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"time"
)

// XrayManager manages the xray-core subprocess for VLESS connections.
type XrayManager struct {
	mu         sync.Mutex
	process    *exec.Cmd
	configPath string
	socksPort  int
	running    bool
}

// VLESSParams holds VLESS connection parameters parsed from a vless:// URI.
type VLESSParams struct {
	UUID        string
	Host        string
	Port        string
	Security    string
	SNI         string
	Fingerprint string
	PublicKey   string
	ShortID     string
	SpiderX     string
	Flow        string
	Network     string
}

// NewXrayManager creates a new manager for xray-core subprocess.
func NewXrayManager() *XrayManager {
	return &XrayManager{
		socksPort: 10808,
	}
}

// Start launches xray-core with a generated config for the given VLESS URI.
func (m *XrayManager) Start(vlessURI string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if m.running {
		return fmt.Errorf("xray-core is already running")
	}

	// Parse VLESS URI
	params, err := ParseVLESSURI(vlessURI)
	if err != nil {
		return fmt.Errorf("failed to parse VLESS URI: %w", err)
	}

	// Generate xray config
	config := m.generateConfig(params)

	// Write config to temp file
	configDir, err := os.UserConfigDir()
	if err != nil {
		configDir = os.TempDir()
	}
	configDir = filepath.Join(configDir, "DrFrakeVPN")
	os.MkdirAll(configDir, 0755)

	m.configPath = filepath.Join(configDir, "xray_config.json")
	if err := os.WriteFile(m.configPath, []byte(config), 0600); err != nil {
		return fmt.Errorf("failed to write xray config: %w", err)
	}

	// Find xray binary
	xrayBin := m.findXrayBinary()
	if xrayBin == "" {
		return fmt.Errorf("xray-core binary not found. Please place xray.exe in the application directory")
	}

	// Start xray-core
	m.process = exec.Command(xrayBin, "run", "-c", m.configPath)
	m.process.Stdout = os.Stdout
	m.process.Stderr = os.Stderr

	if err := m.process.Start(); err != nil {
		return fmt.Errorf("failed to start xray-core: %w", err)
	}

	m.running = true
	log.Printf("[Xray] Started xray-core (PID %d) with SOCKS5 on 127.0.0.1:%d", m.process.Process.Pid, m.socksPort)

	// Wait a moment for xray to start listening
	time.Sleep(500 * time.Millisecond)

	return nil
}

// Stop terminates the xray-core subprocess.
func (m *XrayManager) Stop() error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if !m.running || m.process == nil {
		return nil
	}

	log.Printf("[Xray] Stopping xray-core...")

	if m.process.Process != nil {
		m.process.Process.Kill()
		m.process.Wait()
	}

	m.running = false
	m.process = nil

	// Clean up config file
	if m.configPath != "" {
		os.Remove(m.configPath)
	}

	return nil
}

// GetSOCKS5Config returns the local SOCKS5 address for Outline SDK to use.
func (m *XrayManager) GetSOCKS5Config() string {
	return fmt.Sprintf("socks5://127.0.0.1:%d", m.socksPort)
}

// IsRunning returns whether xray-core is currently running.
func (m *XrayManager) IsRunning() bool {
	m.mu.Lock()
	defer m.mu.Unlock()
	return m.running
}

// findXrayBinary searches for xray-core executable in common locations.
func (m *XrayManager) findXrayBinary() string {
	binaryName := "xray"
	if runtime.GOOS == "windows" {
		binaryName = "xray.exe"
	}

	// Search locations
	locations := []string{
		// Same directory as the application
		filepath.Join(".", binaryName),
		// In xray subdirectory
		filepath.Join(".", "xray", binaryName),
		// User config directory
		func() string {
			dir, _ := os.UserConfigDir()
			return filepath.Join(dir, "DrFrakeVPN", binaryName)
		}(),
	}

	// Also check in PATH
	if path, err := exec.LookPath(binaryName); err == nil {
		return path
	}

	for _, loc := range locations {
		if _, err := os.Stat(loc); err == nil {
			absPath, _ := filepath.Abs(loc)
			return absPath
		}
	}

	return ""
}

// generateConfig creates an xray-core JSON config for a VLESS+Reality connection.
func (m *XrayManager) generateConfig(params *VLESSParams) string {
	config := map[string]interface{}{
		"log": map[string]interface{}{
			"loglevel": "warning",
		},
		"inbounds": []map[string]interface{}{
			{
				"tag":      "socks-in",
				"port":     m.socksPort,
				"listen":   "127.0.0.1",
				"protocol": "socks",
				"settings": map[string]interface{}{
					"auth": "noauth",
					"udp":  true,
				},
				"sniffing": map[string]interface{}{
					"enabled":      true,
					"destOverride": []string{"http", "tls"},
				},
			},
		},
		"outbounds": []map[string]interface{}{
			{
				"tag":      "vless-out",
				"protocol": "vless",
				"settings": map[string]interface{}{
					"vnext": []map[string]interface{}{
						{
							"address": params.Host,
							"port":    params.Port,
							"users": []map[string]interface{}{
								{
									"id":         params.UUID,
									"flow":       params.Flow,
									"encryption": "none",
								},
							},
						},
					},
				},
				"streamSettings": m.buildStreamSettings(params),
			},
			{
				"tag":      "direct",
				"protocol": "freedom",
			},
		},
	}

	data, _ := json.MarshalIndent(config, "", "  ")
	return string(data)
}

// buildStreamSettings creates the streamSettings for xray config.
func (m *XrayManager) buildStreamSettings(params *VLESSParams) map[string]interface{} {
	network := params.Network
	if network == "" {
		network = "tcp"
	}

	ss := map[string]interface{}{
		"network":  network,
		"security": params.Security,
	}

	if params.Security == "reality" {
		ss["realitySettings"] = map[string]interface{}{
			"serverName":  params.SNI,
			"fingerprint": params.Fingerprint,
			"publicKey":   params.PublicKey,
			"shortId":     params.ShortID,
			"spiderX":     params.SpiderX,
		}
	} else if params.Security == "tls" {
		ss["tlsSettings"] = map[string]interface{}{
			"serverName":  params.SNI,
			"fingerprint": params.Fingerprint,
		}
	}

	return ss
}

// ParseVLESSURI parses a vless:// URI into VLESSParams.
func ParseVLESSURI(uri string) (*VLESSParams, error) {
	if !strings.HasPrefix(uri, "vless://") {
		return nil, fmt.Errorf("not a VLESS URI: %s", uri)
	}

	// vless://UUID@HOST:PORT?params#fragment
	u, err := url.Parse(uri)
	if err != nil {
		return nil, fmt.Errorf("failed to parse URI: %w", err)
	}

	params := &VLESSParams{
		UUID: u.User.Username(),
		Host: u.Hostname(),
		Port: u.Port(),
	}

	q := u.Query()
	params.Security = q.Get("security")
	params.SNI = q.Get("sni")
	params.Fingerprint = q.Get("fp")
	params.PublicKey = q.Get("pbk")
	params.ShortID = q.Get("sid")
	params.SpiderX = q.Get("spx")
	params.Flow = q.Get("flow")
	params.Network = q.Get("type")

	if params.Flow == "" {
		params.Flow = "xtls-rprx-vision"
	}
	if params.Security == "" {
		params.Security = "reality"
	}
	if params.Fingerprint == "" {
		params.Fingerprint = "chrome"
	}

	return params, nil
}
