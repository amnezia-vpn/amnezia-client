package main

import (
	"bufio"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"log"
	"net"
	"os/exec"
	"strings"
	"sync"

	"github.com/amnezia-vpn/amneziawg-go/conn"
	"github.com/amnezia-vpn/amneziawg-go/device"
	"github.com/amnezia-vpn/amneziawg-go/tun"
	"golang.org/x/sys/windows"
)

type AWGManager struct {
	mu        sync.Mutex
	running   bool
	device    *device.Device
	tunDevice tun.Device
	tunName   string
	serverIP  string
	tunIP     string
}

func NewAWGManager() *AWGManager {
	return &AWGManager{
		tunName: "DrFrakeVPN",
	}
}

// Start sets up the Wintun adapter and configures it with the given AmneziaWG text config.
func (m *AWGManager) Start(configText string, apiHostIP string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	log.Printf("[AWG] Using NEW b64-to-hex manager v2")

	if m.running {
		return fmt.Errorf("AWG already running")
	}

	// 1. Parse Config
	uapi, serverEndpoint, tunIP, err := parseAmneziaConfigToUAPI(configText)
	if err != nil {
		return fmt.Errorf("failed to parse config: %w", err)
	}

	m.serverIP = serverEndpoint
	m.tunIP = tunIP

	// 2. Create TUN
	log.Printf("[AWG] Creating Wintun adapter: %s", m.tunName)
	tunDev, err := tun.CreateTUN(m.tunName, 0)
	if err != nil {
		return fmt.Errorf("failed to create TUN device: %w", err)
	}
	m.tunDevice = tunDev

	// 3. Initialize Device
	logger := device.NewLogger(device.LogLevelError, "(awg) ")
	m.device = device.NewDevice(m.tunDevice, conn.NewDefaultBind(), logger)
	if err := m.device.Up(); err != nil {
		m.tunDevice.Close()
		return fmt.Errorf("failed to up device: %w", err)
	}

	// 4. Set UAPI Configuration
	err = m.device.IpcSet(uapi)
	if err != nil {
		m.device.Close()
		return fmt.Errorf("failed to set IPC config: %w", err)
	}

	log.Printf("[AWG] AmneziaWG Device configured successfully")

	// 5. Configure Windows IP routing
	if err := m.setupAdapterAndRoutes(apiHostIP); err != nil {
		m.device.Close()
		return fmt.Errorf("failed to setup routes: %w", err)
	}

	m.running = true
	return nil
}

func (m *AWGManager) Stop(apiHostIP string) error {
	m.mu.Lock()
	defer m.mu.Unlock()

	if !m.running {
		return nil
	}

	log.Println("[AWG] Stopping AmneziaWG...")

	// Clean up routes first
	m.cleanupRoutes(apiHostIP)

	if m.device != nil {
		m.device.Close()
		m.device = nil
	}

	m.running = false
	log.Println("[AWG] Stopped.")
	return nil
}

// Configure adapter and bypass routes via PowerShell
func (m *AWGManager) setupAdapterAndRoutes(apiHostIP string) error {
	log.Printf("[AWG] Configuring Adapter: %s IP: %s (Bypass: %s, %s)", m.tunName, m.tunIP, m.serverIP, apiHostIP)

	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "Stop";
		$adapterName = "%s";
		$tunIP = "%s";
		$serverIP = "%s";
		$apiHostIP = "%s";

		# Strip CIDR from TUN IP for NetIPAddress
		$ipOnly = $tunIP -replace "/.*",""

		# Find Default Gateway
		$defRoute = Get-NetRoute -DestinationPrefix "0.0.0.0/0" |
			Where-Object { $_.InterfaceAlias -ne $adapterName } |
			Sort-Object -Property RouteMetric | Select-Object -First 1
		if (!$defRoute) { Write-Error "No default gateway found"; exit 1 }
		$gw = $defRoute.NextHop
		$ifIndex = $defRoute.InterfaceIndex

		# Configure TUN
		$tunIf = Get-NetAdapter -Name $adapterName
		$tunIdx = $tunIf.InterfaceIndex
		Remove-NetIPAddress -InterfaceIndex $tunIdx -Confirm:$false -ErrorAction SilentlyContinue
		New-NetIPAddress -InterfaceIndex $tunIdx -IPAddress $ipOnly -PrefixLength 24 -ErrorAction SilentlyContinue
		Set-NetIPInterface -InterfaceIndex $tunIdx -InterfaceMetric 1 -NlMtuBytes 1280 -ErrorAction SilentlyContinue
		Set-DnsClientServerAddress -InterfaceIndex $tunIdx -ServerAddresses ("1.1.1.1", "8.8.8.8")

		# Bypass Server
		if ($serverIP -ne "") {
			if (!(Get-NetRoute -DestinationPrefix "$serverIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$serverIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# Bypass API
		if ($apiHostIP -ne "" -and $apiHostIP -ne $serverIP) {
			if (!(Get-NetRoute -DestinationPrefix "$apiHostIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$apiHostIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# Route all through VPN
		function Add-RouteIfMissing($prefix, $idx) {
			if (!(Get-NetRoute -DestinationPrefix $prefix -InterfaceIndex $idx -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix $prefix -InterfaceIndex $idx -RouteMetric 1
			}
		}
		Add-RouteIfMissing "1.1.1.1/32" $tunIdx
		Add-RouteIfMissing "8.8.8.8/32" $tunIdx
		Add-RouteIfMissing "0.0.0.0/1" $tunIdx
		Add-RouteIfMissing "128.0.0.0/1" $tunIdx

		Clear-DnsClientCache -ErrorAction SilentlyContinue
	`, m.tunName, m.tunIP, m.serverIP, apiHostIP)

	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("powershell routing failed: %v, out: %s", err, string(out))
	}
	return nil
}

func (m *AWGManager) cleanupRoutes(apiHostIP string) {
	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "SilentlyContinue";
		Remove-NetRoute -DestinationPrefix "1.1.1.1/32" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "8.8.8.8/32" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "0.0.0.0/1" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "128.0.0.0/1" -Confirm:$false
		if ("%s" -ne "") { Remove-NetRoute -DestinationPrefix "%s/32" -Confirm:$false }
		if ("%s" -ne "") { Remove-NetRoute -DestinationPrefix "%s/32" -Confirm:$false }
	`, m.serverIP, m.serverIP, apiHostIP, apiHostIP)

	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	cmd.Run()
}

// b64ToHex converts a standard Wireguard base64 key into a hex string for UAPI.
func b64ToHex(b64 string) string {
	decoded, err := base64.StdEncoding.DecodeString(b64)
	if err != nil {
		log.Printf("[AWG] B64 DECODE ERROR for '%s': %v", b64, err)
		return b64 // fall back to raw string if not valid base64
	}
	encoded := hex.EncodeToString(decoded)
	log.Printf("[AWG] Successfully decoded B64 to Hex")
	return encoded
}

// Parse Amnezia Config to wireguard-go UAPI string format
func parseAmneziaConfigToUAPI(conf string) (string, string, string, error) {
	scanner := bufio.NewScanner(strings.NewReader(conf))
	var uapi strings.Builder

	var serverEndpoint, tunIP string

	section := ""
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" || strings.HasPrefix(line, "#") {
			continue
		}

		if strings.HasPrefix(line, "[") && strings.HasSuffix(line, "]") {
			section = strings.ToLower(line)
			if section == "[peer]" {
				uapi.WriteString("public_key=")
			}
			continue
		}

		parts := strings.SplitN(line, "=", 2)
		if len(parts) != 2 {
			continue
		}

		key := strings.TrimSpace(parts[0])
		val := strings.TrimSpace(parts[1])

		if section == "[interface]" {
			switch strings.ToLower(key) {
			case "privatekey":
				uapi.WriteString("private_key=" + b64ToHex(val) + "\n")
			case "address":
				tunIP = val
			// AmneziaWG specific fields:
			case "jc":
				uapi.WriteString("jc=" + val + "\n")
			case "jmin":
				uapi.WriteString("jmin=" + val + "\n")
			case "jmax":
				uapi.WriteString("jmax=" + val + "\n")
			case "s1":
				uapi.WriteString("s1=" + val + "\n")
			case "s2":
				uapi.WriteString("s2=" + val + "\n")
			case "h1":
				uapi.WriteString("h1=" + val + "\n")
			case "h2":
				uapi.WriteString("h2=" + val + "\n")
			case "h3":
				uapi.WriteString("h3=" + val + "\n")
			case "h4":
				uapi.WriteString("h4=" + val + "\n")
			}
		} else if section == "[peer]" {
			switch strings.ToLower(key) {
			case "publickey":
				// Already wrote key prefix when section started. Overwrite to ensure it's set right before other peer props
				// UAPI allows multiple peers by separating them with public_key=...
				uapi.WriteString(b64ToHex(val) + "\n")
			case "presharedkey":
				uapi.WriteString("preshared_key=" + b64ToHex(val) + "\n")
			case "allowedips":
				ips := strings.Split(val, ",")
				for _, ip := range ips {
					uapi.WriteString("allowed_ip=" + strings.TrimSpace(ip) + "\n")
				}
			case "endpoint":
				uapi.WriteString("endpoint=" + val + "\n")
				host, _, _ := net.SplitHostPort(val)
				if host != "" {
					serverEndpoint = host
				} else {
					serverEndpoint = val
				}
			case "persistentkeepalive":
				uapi.WriteString("persistent_keepalive_interval=" + val + "\n")
			}
		}
	}

	return uapi.String(), serverEndpoint, tunIP, nil
}
