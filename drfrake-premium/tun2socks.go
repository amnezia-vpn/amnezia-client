package main

import (
	"fmt"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"time"

	"golang.org/x/sys/windows"
)

// Tun2SocksManager manages the tun2socks subprocess.
type Tun2SocksManager struct {
	cmd     *exec.Cmd
	tunName string
}

func NewTun2SocksManager() *Tun2SocksManager {
	return &Tun2SocksManager{
		tunName: "DrFrakeVPN",
	}
}

// Start launches tun2socks.exe to create a TUN adapter bridged to a SOCKS5 proxy.
func (m *Tun2SocksManager) Start(socksAddr string) error {
	exePath, err := m.findExe()
	if err != nil {
		return fmt.Errorf("tun2socks.exe not found: %w. Download from https://github.com/xjasonlyu/tun2socks/releases", err)
	}

	m.cmd = exec.Command(exePath,
		"-device", "tun://"+m.tunName,
		"-proxy", "socks5://"+socksAddr,
		"-loglevel", "warn",
	)
	m.cmd.Stdout = os.Stdout
	m.cmd.Stderr = os.Stderr
	m.cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}

	if err := m.cmd.Start(); err != nil {
		return fmt.Errorf("failed to start tun2socks: %w", err)
	}

	log.Printf("[Tun2Socks] Started (PID %d), TUN: %s, SOCKS5: %s", m.cmd.Process.Pid, m.tunName, socksAddr)

	// Wait for TUN adapter to appear
	if err := m.waitForAdapter(); err != nil {
		m.Stop()
		return fmt.Errorf("TUN adapter didn't appear: %w", err)
	}

	return nil
}

// Stop kills the tun2socks process.
func (m *Tun2SocksManager) Stop() {
	if m.cmd != nil && m.cmd.Process != nil {
		log.Println("[Tun2Socks] Stopping...")
		m.cmd.Process.Kill()
		m.cmd.Wait()
		m.cmd = nil
		log.Println("[Tun2Socks] Stopped.")
	}
}

// waitForAdapter waits for the TUN adapter to appear in Windows.
func (m *Tun2SocksManager) waitForAdapter() error {
	for i := 0; i < 30; i++ {
		time.Sleep(500 * time.Millisecond)

		// Check if adapter exists
		psCmd := fmt.Sprintf(`Get-NetAdapter -Name "%s" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Status`, m.tunName)
		cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
		cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
		out, err := cmd.Output()
		if err == nil && len(out) > 0 {
			log.Printf("[Tun2Socks] TUN adapter '%s' is ready (attempt %d)", m.tunName, i+1)
			return nil
		}
	}
	return fmt.Errorf("timeout waiting for adapter %s", m.tunName)
}

// ConfigureAdapter sets IP address on the TUN adapter.
func (m *Tun2SocksManager) ConfigureAdapter(tunIP string) error {
	psCmd := fmt.Sprintf(`
		$adapterName = "%s"
		$tunIP = "%s"

		# Set IP address
		$adapter = Get-NetAdapter -Name $adapterName
		$ifIndex = $adapter.InterfaceIndex

		# Remove existing IPs
		Remove-NetIPAddress -InterfaceIndex $ifIndex -Confirm:$false -ErrorAction SilentlyContinue

		# Set new IP
		New-NetIPAddress -InterfaceIndex $ifIndex -IPAddress $tunIP -PrefixLength 24 -ErrorAction SilentlyContinue

		# Force highest priority metric so Windows prefers TUN over physical interface
		Set-NetIPInterface -InterfaceIndex $ifIndex -InterfaceMetric 1 -NlMtuBytes 1350 -ErrorAction SilentlyContinue

		# Set DNS on TUN so all DNS goes through the VPN
		Set-DnsClientServerAddress -InterfaceIndex $ifIndex -ServerAddresses ("1.1.1.1", "8.8.8.8")
	`, m.tunName, tunIP)

	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("failed to configure adapter: %v, output: %s", err, string(out))
	}
	log.Printf("[Tun2Socks] Adapter configured: IP=%s", tunIP)
	return nil
}

// SetupRoutes configures routing to send traffic through the TUN.
func (m *Tun2SocksManager) SetupRoutes(serverIP, tunIP, apiHostIP string) error {
	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "Stop";
		$serverIP = "%s";
		$tunIP = "%s";
		$apiHostIP = "%s";
		$adapterName = "%s";

		# 1. Find Default Gateway
		$defRoute = Get-NetRoute -DestinationPrefix "0.0.0.0/0" |
			Where-Object { $_.InterfaceAlias -ne $adapterName } |
			Sort-Object -Property RouteMetric | Select-Object -First 1
		if (!$defRoute) { Write-Error "No default gateway found"; exit 1 }
		$gw = $defRoute.NextHop
		$ifIndex = $defRoute.InterfaceIndex

		# 2. Server bypass: route VPN server via old gateway
		if ($serverIP -ne "") {
			if (!(Get-NetRoute -DestinationPrefix "$serverIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$serverIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# 3. API Bypass: route Backend API via old gateway
		if ($apiHostIP -ne "" -and $apiHostIP -ne $serverIP) {
			if (!(Get-NetRoute -DestinationPrefix "$apiHostIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$apiHostIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# (DNS bypass removed so DNS correctly falls into the TUN routes)


		# 4. Route all traffic via TUN
		$tunIf = Get-NetAdapter -Name $adapterName
		$tunIdx = $tunIf.InterfaceIndex

		function Add-RouteIfMissing($prefix, $idx) {
			if (!(Get-NetRoute -DestinationPrefix $prefix -InterfaceIndex $idx -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix $prefix -InterfaceIndex $idx -RouteMetric 1
			}
		}

		Add-RouteIfMissing "1.1.1.1/32" $tunIdx
		Add-RouteIfMissing "8.8.8.8/32" $tunIdx
		Add-RouteIfMissing "0.0.0.0/1" $tunIdx
		Add-RouteIfMissing "128.0.0.0/1" $tunIdx

		# Flush DNS cache to force Windows to use the TUN adapter immediately
		Clear-DnsClientCache -ErrorAction SilentlyContinue
	`, serverIP, tunIP, apiHostIP, m.tunName)

	log.Printf("[Tun2Socks] Setting up routes: server=%s, tun=%s...", serverIP, tunIP)
	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("failed to setup routes: %v, output: %s", err, string(out))
	}
	log.Println("[Tun2Socks] Routes configured successfully.")
	return nil
}

// CleanupRoutes removes VPN routes.
func (m *Tun2SocksManager) CleanupRoutes(serverIP string) {
	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "SilentlyContinue";
		Remove-NetRoute -DestinationPrefix "1.1.1.1/32" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "8.8.8.8/32" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "0.0.0.0/1" -Confirm:$false
		Remove-NetRoute -DestinationPrefix "128.0.0.0/1" -Confirm:$false
		if ("%s" -ne "") {
			Remove-NetRoute -DestinationPrefix "%s/32" -Confirm:$false
		}
	`, serverIP, serverIP)

	log.Println("[Tun2Socks] Cleaning up routes...")
	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	cmd.CombinedOutput()
	log.Println("[Tun2Socks] Routes cleaned up.")
}

// findExe locates tun2socks.exe.
func (m *Tun2SocksManager) findExe() (string, error) {
	// Check AppData
	appData := os.Getenv("APPDATA")
	exePath := filepath.Join(appData, "DrFrakeVPN", "tun2socks.exe")
	if _, err := os.Stat(exePath); err == nil {
		return exePath, nil
	}

	// Check next to executable
	exe, _ := os.Executable()
	exePath = filepath.Join(filepath.Dir(exe), "tun2socks.exe")
	if _, err := os.Stat(exePath); err == nil {
		return exePath, nil
	}

	return "", fmt.Errorf("tun2socks.exe not found in %s or %s", filepath.Join(appData, "DrFrakeVPN"), filepath.Dir(exe))
}
