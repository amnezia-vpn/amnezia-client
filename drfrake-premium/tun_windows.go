package main

import (
	"fmt"
	"log"
	"os/exec"
	"strings"
	"time"

	"golang.org/x/sys/windows"
	"golang.zx2c4.com/wintun"
)

const (
	driverName  = "Wintun"
	adapterName = "DrFrakeVPN"
	mtu         = 1500
)

type WindowsTUN struct {
	adapter *wintun.Adapter
	session wintun.Session
}

func NewWindowsTUN() (*WindowsTUN, error) {
	log.Println("[Wintun] Initializing...")

	// Best Practice: cleanup stale adapter with the same name before creating a new one.
	// This prevents "Element not found" errors when Wintun tries to resolve name collisions (e.g. "DrFrakeVPN 1").
	if oldAdapter, err := wintun.OpenAdapter(adapterName); err == nil {
		log.Println("[Wintun] Found existing adapter, closing/deleting it...")
		oldAdapter.Close()
	} else {
		log.Println("[Wintun] No existing adapter found (clean slate).")
	}

	// Create adapter. Using nil GUID for random/auto-generated.
	log.Println("[Wintun] Creating new adapter...")
	adapter, err := wintun.CreateAdapter(adapterName, "DrFrakeVPN", nil)
	if err != nil {
		log.Printf("[Wintun] CreateAdapter failed: %v", err)
		return nil, fmt.Errorf("failed to create Wintun adapter: %w", err)
	}
	log.Println("[Wintun] Adapter created successfully.")

	// Capacity: 0x400000 (4MB) is a common default.
	// Wintun uses ring buffer size.
	log.Println("[Wintun] Starting session...")
	session, err := adapter.StartSession(0x400000)
	if err != nil {
		log.Printf("[Wintun] StartSession failed: %v", err)
		adapter.Close()
		return nil, fmt.Errorf("failed to start Wintun session: %w", err)
	}
	log.Println("[Wintun] Session started.")

	return &WindowsTUN{
		adapter: adapter,
		session: session,
	}, nil
}

func (t *WindowsTUN) Read(p []byte) (int, error) {
	pkt, err := t.session.ReceivePacket()
	if err != nil {
		return 0, err
	}
	n := copy(p, pkt)
	t.session.ReleaseReceivePacket(pkt)
	return n, nil
}

func (t *WindowsTUN) Write(p []byte) (int, error) {
	pkt, err := t.session.AllocateSendPacket(len(p))
	if err != nil {
		return 0, err
	}
	copy(pkt, p)
	t.session.SendPacket(pkt)
	return len(p), nil
}

func (t *WindowsTUN) Close() error {
	t.session.End()
	return t.adapter.Close()
}

func (t *WindowsTUN) MTU() int {
	return mtu
}

func (t *WindowsTUN) Configure(localIP string) error {
	log.Printf("[Wintun] Configuring IP %s via netsh... (Looping 10s)", localIP)

	var lastErr error
	var lastOut string

	// Loop for up to 10 seconds to allow interface to appear
	for i := 0; i < 20; i++ {
		// 1. Check if specific interface exists in netsh
		checkCmd := exec.Command("netsh", "interface", "ip", "show", "address", fmt.Sprintf("name=\"%s\"", adapterName))
		checkCmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
		checkOut, _ := checkCmd.CombinedOutput()
		output := string(checkOut)

		// If verify shows IP is already there -> Success
		if strings.Contains(output, localIP) {
			log.Printf("[Wintun] Success: IP %s already present on attempt %d.", localIP, i+1)
			return nil
		}

		// 2. Try to set IP
		cmd := exec.Command("netsh", "interface", "ip", "set", "address",
			fmt.Sprintf("name=\"%s\"", adapterName),
			"source=static",
			fmt.Sprintf("addr=%s", localIP),
			"mask=255.255.255.0",
			"gateway=none")
		cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
		out, err := cmd.CombinedOutput()

		if err == nil {
			log.Printf("[Wintun] Success: IP configured on attempt %d.", i+1)
			return nil
		}

		lastErr = err
		lastOut = string(out)

		// Check for "Already exists" error in various languages (Russian, English)
		// "Этот объект уже существует" / "The object already exists"
		if strings.Contains(lastOut, "существует") || strings.Contains(lastOut, "exists") {
			log.Printf("[Wintun] 'Object exists' error detected. Assuming success and proceeding.")
			return nil
		}

		// Log intermediate failure quietly
		if i%5 == 0 {
			log.Printf("[Wintun] Attempt %d/20 failed: %v | Out: %s", i+1, err, strings.TrimSpace(lastOut))
		}

		time.Sleep(500 * time.Millisecond)
	}

	return fmt.Errorf("failed to configure IP after 10s. Last error: %v, Output: %s", lastErr, lastOut)
}

func (t *WindowsTUN) SetupRoutes(serverIP string, localTUNIP string) error {
	// PowerShell script to setup routing:
	// 1. Find Default Gateway
	// 2. Add route to VPN Server via Default Gateway (Loop prevention)
	// 3. Add 0.0.0.0/1 and 128.0.0.0/1 via TUN (Redirect traffic)

	psCmd := fmt.Sprintf(`
		$ErrorActionPreference = "Stop";
		$serverIP = "%s";
		$tunIP = "%s";
		
		# 1. Find Default Gateway (metric based)
		$defRoute = Get-NetRoute -DestinationPrefix "0.0.0.0/0" | Sort-Object -Property RouteMetric | Select-Object -First 1
		if (!$defRoute) { Write-Error "No default gateway found"; exit 1 }
		$gw = $defRoute.NextHop
		$ifIndex = $defRoute.InterfaceIndex
		
		# 2. Prevent Loop: Route to VPN Server via old gateway
		if ($serverIP -ne "") {
			if (!(Get-NetRoute -DestinationPrefix "$serverIP/32" -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix "$serverIP/32" -NextHop $gw -InterfaceIndex $ifIndex -RouteMetric 1
			}
		}

		# 3. Route Traffic via TUN
		$tunIf = Get-NetIPAddress -IPAddress $tunIP
		if (!$tunIf) { Write-Error "TUN Interface not found"; exit 1 }
		$tunIdx = $tunIf.InterfaceIndex
		
		# Helper to add route if missing
		function Add-Route($prefix, $idx) {
			if (!(Get-NetRoute -DestinationPrefix $prefix -ErrorAction SilentlyContinue)) {
				New-NetRoute -DestinationPrefix $prefix -InterfaceIndex $idx -RouteMetric 1
			}
		}
		
		Add-Route "0.0.0.0/1" $tunIdx
		Add-Route "128.0.0.0/1" $tunIdx
	`, serverIP, localTUNIP)

	log.Printf("[Routing] Configuring routes for Server: %s, TUN: %s...", serverIP, localTUNIP)
	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}

	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("failed to setup routes: %v, output: %s", err, string(out))
	}
	log.Println("[Routing] Routes configured successfully.")
	return nil
}
