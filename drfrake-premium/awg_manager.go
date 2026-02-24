package main

import (
	"bufio"
	"encoding/base64"
	"encoding/hex"
	"fmt"
	"log"
	"net"
	"net/netip"
	"os/exec"
	"strings"
	"sync"
	"time"

	"github.com/amnezia-vpn/amneziawg-go/conn"
	"github.com/amnezia-vpn/amneziawg-go/device"
	"github.com/amnezia-vpn/amneziawg-go/tun"
	"golang.org/x/sys/windows"
	"golang.zx2c4.com/wireguard/windows/tunnel/winipcfg"
)

type luidGetter interface {
	LUID() uint64
}

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
	log.Printf("[AWG] Creating Wintun adapter: %s (MTU 1280)", m.tunName)
	tunDev, err := tun.CreateTUN(m.tunName, 1280)
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

	// DIAGNOSTIC LOGGER: Dump actual Tunnel statistics to verify if Handshake is succeeding
	go func() {
		for {
			time.Sleep(3 * time.Second)
			m.mu.Lock()
			isRunning := m.running
			m.mu.Unlock()
			if !isRunning {
				return
			}
			out, err := m.device.IpcGet()
			if err != nil {
				log.Printf("[AWG-Stats] Error reading stats: %v", err)
			} else {
				rx := "0"
				tx := "0"
				hs := "0"
				for _, line := range strings.Split(out, "\n") {
					if strings.HasPrefix(line, "rx_bytes=") {
						rx = strings.TrimPrefix(line, "rx_bytes=")
					} else if strings.HasPrefix(line, "tx_bytes=") {
						tx = strings.TrimPrefix(line, "tx_bytes=")
					} else if strings.HasPrefix(line, "last_handshake_time_sec=") {
						hs = strings.TrimPrefix(line, "last_handshake_time_sec=")
					}
				}
				if hs != "0" && hs != "" {
					log.Printf("[AWG-Stats] TX: %s bytes | RX: %s bytes | Handshake SEC: %s", tx, rx, hs)
				} else {
					log.Printf("[AWG-Stats] TX: %s bytes | RX: %s bytes | (No Handshake yet)", tx, rx)
				}
			}
		}
	}()

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

func (m *AWGManager) setupAdapterAndRoutes(apiHostIP string) error {
	log.Printf("[AWG] Configuring Adapter natively via WinIPCfg: %s IP: %s (Bypass: %s, %s)", m.tunName, m.tunIP, m.serverIP, apiHostIP)

	var luid winipcfg.LUID

	// Fast Path: Extract LUID directly from amnezia tun.NativeTun
	if getter, ok := m.tunDevice.(luidGetter); ok {
		luid = winipcfg.LUID(getter.LUID())
		log.Printf("[AWG] Natively recovered Wintun LUID: %d", luid)
	}

	// Fallback Path: Iterate NDIS tree using the requested Device alias
	if luid == 0 {
		aas, err := winipcfg.GetAdaptersAddresses(windows.AF_UNSPEC, winipcfg.GAAFlagIncludePrefix)
		if err == nil {
			for _, aa := range aas {
				if aa.FriendlyName() == m.tunName {
					luid = aa.LUID
					break
				}
			}
		}
	}

	if luid == 0 {
		return fmt.Errorf("could not find LUID for adapter %s", m.tunName)
	}

	// 1. Set IP Address (This natively configures NDIS correctly)
	ip, err := netip.ParsePrefix(m.tunIP)
	if err != nil {
		return fmt.Errorf("invalid tun IP %s: %v", m.tunIP, err)
	}
	err = luid.SetIPAddresses([]netip.Prefix{ip})
	if err != nil {
		return fmt.Errorf("failed to set IP address: %w", err)
	}

	// 2. Set MTU explicitly for Windows networking stack to match Amnezia WG packet padding (1280)
	// Wintun natively inherits the 1280 MTU passed into tun.CreateTUN above.

	// 3. Set DNS
	dns1, _ := netip.ParseAddr("1.1.1.1")
	dns2, _ := netip.ParseAddr("8.8.8.8")
	err = luid.SetDNS(windows.AF_INET, []netip.Addr{dns1, dns2}, []string{})
	if err != nil {
		return fmt.Errorf("failed to set DNS: %w", err)
	}

	// 4. Set Routes (Standard Wireguard equivalent of 0.0.0.0/0)
	routes := []*winipcfg.RouteData{
		{Destination: netip.MustParsePrefix("0.0.0.0/1"), NextHop: netip.IPv4Unspecified(), Metric: 0},
		{Destination: netip.MustParsePrefix("128.0.0.0/1"), NextHop: netip.IPv4Unspecified(), Metric: 0},
	}
	err = luid.AddRoutes(routes)
	if err != nil {
		return fmt.Errorf("failed to add default route: %w", err)
	}

	// 5. Bypass physical routes for VPN server and API via powershell
	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "Stop";
		$adapterName = "%s";
		$serverIP = "%s";
		$apiHostIP = "%s";

		# Find Default Gateway on non-vpn adapters
		$defRoute = Get-NetRoute -DestinationPrefix "0.0.0.0/0" |
			Where-Object { $_.InterfaceAlias -ne $adapterName } |
			Sort-Object -Property RouteMetric | Select-Object -First 1
		if (!$defRoute) { Write-Error "No default gateway found"; exit 1 }
		$gw = $defRoute.NextHop
		$ifIndex = $defRoute.InterfaceIndex

		if ($serverIP -ne "") {
			if (!(Get-NetRoute -DestinationPrefix "$serverIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$serverIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		if ($apiHostIP -ne "" -and $apiHostIP -ne $serverIP) {
			if (!(Get-NetRoute -DestinationPrefix "$apiHostIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$apiHostIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# Allow TCP traffic by explicitly marking as Private Network
		Start-Sleep -Milliseconds 500
		$tunIf = Get-NetAdapter -Name $adapterName -ErrorAction SilentlyContinue
		if ($tunIf) {
			Set-NetConnectionProfile -InterfaceIndex $tunIf.InterfaceIndex -NetworkCategory Private -ErrorAction SilentlyContinue
		}
		
		Clear-DnsClientCache -ErrorAction SilentlyContinue
	`, m.tunName, m.serverIP, apiHostIP)

	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("powershell bypass routing failed: %v, out: %s", err, string(out))
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
