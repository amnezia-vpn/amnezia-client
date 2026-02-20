package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"

	"golang.getoutline.org/sdk/network"
	"golang.getoutline.org/sdk/network/lwip2transport"
	"golang.getoutline.org/sdk/x/configurl"
	"golang.org/x/sys/windows"
)

type Session struct {
	Token string `json:"token"`
	Email string `json:"email"`
	Plan  string `json:"plan"`
}

// App struct
type App struct {
	ctx              context.Context
	tunDevice        *WindowsTUN
	lwipDevice       network.IPDevice
	isConnected      bool
	activeConfig     string
	currentUser      *User
	config           *Config
	apiClient        *APIClient
	authToken        string
	xrayManager      *XrayManager
	tun2socksManager *Tun2SocksManager
	vlessServerIP    string
}

// NewApp creates a new App application struct
func NewApp() *App {
	return &App{}
}

// startup is called when the app starts.
func (a *App) startup(ctx context.Context) {
	a.ctx = ctx

	// Load Config
	var err error
	a.config, err = LoadConfig()
	if err != nil {
		log.Printf("Failed to load config: %v (using defaults)", err)
	}

	// Initialize API Client for backend communication
	backendURL := "http://31.135.65.188:8080"
	if a.config.BackendURL != "" && !strings.Contains(a.config.BackendURL, "localhost") {
		backendURL = a.config.BackendURL
	}
	log.Printf("Using Backend URL: %s", backendURL)
	a.apiClient = NewAPIClient(backendURL)
	log.Printf("API Client initialized: %s", backendURL)

	// Clean up legacy local SQLite database
	dataDir, err := os.UserConfigDir()
	if err == nil {
		dbDir := filepath.Join(dataDir, "DrFrakeVPN")
		dbPath := filepath.Join(dbDir, "drfrake.db")
		os.Remove(dbPath)
	}

	// Restore session
	a.loadSession()
}

func (a *App) getSessionPath() string {
	configDir, _ := os.UserConfigDir()
	return filepath.Join(configDir, "DrFrakeVPN", "session.json")
}

func (a *App) saveSession(token, email, plan string) {
	data, _ := json.Marshal(Session{Token: token, Email: email, Plan: plan})
	os.WriteFile(a.getSessionPath(), data, 0600)
}

func (a *App) loadSession() {
	data, err := os.ReadFile(a.getSessionPath())
	if err != nil {
		return
	}
	var s Session
	if err := json.Unmarshal(data, &s); err != nil {
		return
	}

	// Validate token by calling the backend API
	apiUser, err := a.apiClient.ValidateToken(s.Token)
	if err != nil {
		log.Printf("Session expired or invalid: %v", err)
		a.deleteSession()
		return
	}

	a.authToken = s.Token
	a.currentUser = &User{
		ID:    apiUser.ID,
		Email: s.Email,
	}
	log.Printf("[Auth] Session restored for: %s", s.Email)
}

func (a *App) deleteSession() {
	os.Remove(a.getSessionPath())
}

// shutdown is called when the app quits
func (a *App) shutdown(ctx context.Context) {
	if a.isConnected {
		a.Disconnect()
	}
}

// --- Auth Methods ---

func (a *App) Register(email string, password string) (*User, error) {
	log.Printf("[App] Registering user %s using Backend URL: %s", email, a.apiClient.BaseURL)
	authResp, err := a.apiClient.Register(email, password)
	if err != nil {
		return nil, err
	}

	user := &User{ID: authResp.User.ID, Email: authResp.User.Email}
	a.currentUser = user
	a.authToken = authResp.Token
	a.saveSession(authResp.Token, email, authResp.User.Plan)
	log.Printf("[Auth] User registered via API: %s", email)
	return user, nil
}

func (a *App) Login(email string, password string) (*User, error) {
	log.Printf("[App] Logging in user %s using Backend URL: %s", email, a.apiClient.BaseURL)
	authResp, err := a.apiClient.Login(email, password)
	if err != nil {
		return nil, err
	}

	user := &User{ID: authResp.User.ID, Email: authResp.User.Email}
	a.currentUser = user
	a.authToken = authResp.Token
	a.saveSession(authResp.Token, email, authResp.User.Plan)
	log.Printf("[Auth] User logged in via API: %s", email)
	return user, nil
}

func (a *App) Logout() {
	if a.isConnected {
		a.Disconnect()
	}
	a.currentUser = nil
	a.deleteSession()
}

func (a *App) GetCurrentUser() *User {
	return a.currentUser
}

// --- Server Methods ---

func (a *App) GetServers() []Server {
	// Always fetch from backend API
	if a.apiClient != nil && a.authToken != "" {
		apiServers, err := a.apiClient.GetServers()
		if err == nil {
			var servers []Server
			for _, s := range apiServers {
				servers = append(servers, Server{
					ID:        s.ID,
					Country:   s.Country,
					City:      s.City,
					Flag:      s.Flag,
					Config:    s.Config,
					IsPremium: s.IsPremium,
					Latency:   50,
				})
			}
			log.Printf("[Servers] Loaded %d servers from API", len(servers))
			return servers
		}
		log.Printf("[Servers] API failed: %v", err)
	}

	return []Server{}
}

// --- VPN Methods ---

func (a *App) Connect(config string, serverID string) error {
	if a.currentUser == nil {
		return fmt.Errorf("please login first")
	}

	if a.isConnected {
		return fmt.Errorf("already connected")
	}

	// Check if server is premium and user has access
	servers := a.GetServers()
	for _, s := range servers {
		if s.ID == serverID && s.IsPremium {
			// Get plan from session
			data, err := os.ReadFile(a.getSessionPath())
			if err != nil {
				return fmt.Errorf("failed to check subscription: %w", err)
			}
			var sess Session
			json.Unmarshal(data, &sess)

			if sess.Plan == "" || sess.Plan == "free" {
				return fmt.Errorf("premium subscription required for this server")
			}
		}
	}

	// Mark connected early to prevent double-clicks
	a.isConnected = true

	log.Printf("[VPN] Connecting with config: %s", config)

	var err error
	if strings.HasPrefix(config, "vless://") {
		err = a.connectVLESS(config)
	} else {
		err = a.connectShadowsocks(config)
	}

	if err != nil {
		a.isConnected = false
	}
	return err
}

// connectVLESS connects via xray-core (SOCKS5) + tun2socks (TUN tunneling).
func (a *App) connectVLESS(config string) error {
	log.Printf("[VPN] Detected VLESS protocol, starting xray-core...")

	// Parse VLESS URI to get server host for routing
	vlessParams, err := ParseVLESSURI(config)
	if err != nil {
		return fmt.Errorf("failed to parse VLESS config: %w", err)
	}
	a.vlessServerIP = vlessParams.Host

	// 1. Start xray-core → SOCKS5 proxy
	if a.xrayManager == nil {
		a.xrayManager = NewXrayManager()
	}
	if err := a.xrayManager.Start(config); err != nil {
		return fmt.Errorf("failed to start xray-core: %w", err)
	}

	socksAddr := fmt.Sprintf("127.0.0.1:%d", a.xrayManager.socksPort)

	// 2. Start tun2socks → TUN adapter bridged to SOCKS5
	a.tun2socksManager = NewTun2SocksManager()
	if err := a.tun2socksManager.Start(socksAddr); err != nil {
		a.stopXray()
		return fmt.Errorf("failed to start tun2socks: %w", err)
	}

	// 3. Configure TUN adapter IP
	tunIP := "10.0.85.2"
	if err := a.tun2socksManager.ConfigureAdapter(tunIP); err != nil {
		a.tun2socksManager.Stop()
		a.stopXray()
		return fmt.Errorf("failed to configure TUN: %w", err)
	}

	// 4. Setup routes: bypass server IP + DNS, route all via TUN
	if err := a.tun2socksManager.SetupRoutes(vlessParams.Host, tunIP); err != nil {
		a.tun2socksManager.Stop()
		a.stopXray()
		return fmt.Errorf("failed to setup routes: %w", err)
	}

	log.Printf("[VPN] VLESS connected via TUN (xray SOCKS5: %s)", socksAddr)
	a.activeConfig = config
	return nil
}

// connectShadowsocks connects via TUN + LWIP + Outline SDK (original flow).
func (a *App) connectShadowsocks(config string) error {
	var serverHost string
	if cfg, err := configurl.ParseConfig(config); err == nil {
		serverHost = cfg.URL.Hostname()
	}

	// 1. Create Dialers
	providers := configurl.NewDefaultProviders()
	sd, err := providers.NewStreamDialer(context.Background(), config)
	if err != nil {
		return fmt.Errorf("failed to create stream dialer: %w", err)
	}
	pl, err := providers.NewPacketListener(context.Background(), config)
	if err != nil {
		return fmt.Errorf("failed to create packet listener: %w", err)
	}
	pp, err := network.NewPacketProxyFromPacketListener(pl)
	if err != nil {
		return fmt.Errorf("failed to create packet proxy: %w", err)
	}

	// 2. Create & Configure TUN
	tun, err := NewWindowsTUN()
	if err != nil {
		return fmt.Errorf("failed to create TUN device: %w", err)
	}
	tunIP := "10.0.85.2"
	if err := tun.Configure(tunIP); err != nil {
		tun.Close()
		return fmt.Errorf("failed to configure TUN: %w", err)
	}
	a.tunDevice = tun

	// 2.5 Setup Routing
	if err := tun.SetupRoutes(serverHost, tunIP); err != nil {
		log.Printf("[VPN] Routing setup failed: %v", err)
		tun.Close()
		return fmt.Errorf("failed to setup routes: %w", err)
	}

	// 3. Configure LWIP Stack
	dev, err := lwip2transport.ConfigureDevice(sd, pp)
	if err != nil {
		tun.Close()
		return fmt.Errorf("failed to configure LWIP: %w", err)
	}
	a.lwipDevice = dev

	// 4. Start Packet Forwarding
	go func() {
		_, err := io.Copy(a.tunDevice, a.lwipDevice)
		if err != nil {
			log.Printf("[VPN] Copy LWIP->TUN error: %v", err)
		}
	}()
	go func() {
		_, err := io.Copy(a.lwipDevice, a.tunDevice)
		if err != nil {
			log.Printf("[VPN] Copy TUN->LWIP error: %v", err)
		}
	}()

	log.Println("[VPN] TUN Device started. Routing traffic...")
	a.isConnected = true
	a.activeConfig = config
	return nil
}

func (a *App) Disconnect() error {
	// Stop tun2socks first (removes TUN adapter)
	if a.tun2socksManager != nil {
		a.tun2socksManager.CleanupRoutes(a.vlessServerIP)
		a.tun2socksManager.Stop()
		a.tun2socksManager = nil
	}

	// Remove system proxy
	unsetSystemProxy()

	// Stop Shadowsocks TUN/LWIP if active
	if a.tunDevice != nil {
		a.tunDevice.Close()
		a.tunDevice = nil
	}
	if a.lwipDevice != nil {
		a.lwipDevice.Close()
		a.lwipDevice = nil
	}
	a.stopXray()
	a.isConnected = false
	a.vlessServerIP = ""
	log.Println("[VPN] Disconnected.")
	return nil
}

// stopXray stops the xray-core subprocess if running.
func (a *App) stopXray() {
	if a.xrayManager != nil && a.xrayManager.IsRunning() {
		if err := a.xrayManager.Stop(); err != nil {
			log.Printf("[VPN] Error stopping xray-core: %v", err)
		}
	}
}

// setSystemProxy sets Windows system-wide proxy to route traffic through xray.
func setSystemProxy(host string, port int) error {
	proxy := fmt.Sprintf("%s:%d", host, port)
	psCmd := fmt.Sprintf(`
		$regPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
		Set-ItemProperty -Path $regPath -Name ProxyEnable -Value 1
		Set-ItemProperty -Path $regPath -Name ProxyServer -Value '%s'
		Set-ItemProperty -Path $regPath -Name ProxyOverride -Value '<local>;127.0.0.1;localhost'

		# Notify WinInet so browsers pick up the change immediately
		$sig = '[DllImport("wininet.dll", SetLastError=true)] public static extern bool InternetSetOption(IntPtr h, int o, IntPtr b, int l);'
		$t = Add-Type -MemberDefinition $sig -Name WinInet -Namespace Proxy -PassThru
		$t::InternetSetOption(0, 39, 0, 0)  # INTERNET_OPTION_SETTINGS_CHANGED
		$t::InternetSetOption(0, 37, 0, 0)  # INTERNET_OPTION_REFRESH
	`, proxy)

	log.Printf("[Proxy] Setting system proxy to %s", proxy)
	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		return fmt.Errorf("failed to set proxy: %v, output: %s", err, string(out))
	}
	log.Println("[Proxy] System proxy set and browsers notified.")
	return nil
}

// unsetSystemProxy removes Windows system-wide proxy.
func unsetSystemProxy() {
	psCmd := `
		$regPath = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
		Set-ItemProperty -Path $regPath -Name ProxyEnable -Value 0
		Remove-ItemProperty -Path $regPath -Name ProxyServer -ErrorAction SilentlyContinue

		# Notify WinInet so browsers pick up the change immediately
		$sig = '[DllImport("wininet.dll", SetLastError=true)] public static extern bool InternetSetOption(IntPtr h, int o, IntPtr b, int l);'
		$t = Add-Type -MemberDefinition $sig -Name WinInet2 -Namespace Proxy -PassThru
		$t::InternetSetOption(0, 39, 0, 0)
		$t::InternetSetOption(0, 37, 0, 0)
	`

	log.Println("[Proxy] Removing system proxy...")
	cmd := exec.Command("powershell", "-NoProfile", "-NonInteractive", "-Command", psCmd)
	cmd.SysProcAttr = &windows.SysProcAttr{HideWindow: true}
	if out, err := cmd.CombinedOutput(); err != nil {
		log.Printf("[Proxy] Warning removing proxy: %v, output: %s", err, string(out))
	}
	log.Println("[Proxy] System proxy removed.")
}

func (a *App) IsConnected() bool {
	return a.isConnected
}

// --- Subscription Methods (exposed to React) ---

func (a *App) GetSubscription() (*Subscription, error) {
	if a.currentUser == nil {
		return nil, fmt.Errorf("not logged in")
	}

	data, err := os.ReadFile(a.getSessionPath())
	if err != nil {
		return nil, err
	}
	var sess Session
	json.Unmarshal(data, &sess)

	plan := sess.Plan
	if plan == "" {
		plan = "free"
	}

	return &Subscription{
		UserID: a.currentUser.ID,
		Plan:   PlanType(plan),
		Status: StatusActive,
	}, nil
}

func (a *App) InitPayment(plan string) (*APIPaymentResponse, error) {
	if a.currentUser == nil {
		return nil, fmt.Errorf("not logged in")
	}
	if a.apiClient == nil || a.authToken == "" {
		return nil, fmt.Errorf("not connected to server")
	}
	return a.apiClient.InitPayment(plan)
}

func (a *App) CheckPayment(paymentID string) (string, error) {
	if a.currentUser == nil {
		return "", fmt.Errorf("not logged in")
	}

	status, plan, err := a.apiClient.CheckPayment(paymentID)
	if err != nil {
		return "", err
	}

	// If payment succeeded, update the local session plan
	if status == "succeeded" && plan != "" {
		a.saveSession(a.authToken, a.currentUser.Email, plan)
		log.Printf("[Payment] Upgraded user %s to plan: %s", a.currentUser.Email, plan)
	}

	return status, nil
}

func (a *App) CancelAutoRenew() error {
	return nil
}

func (a *App) EnableAutoRenew() error {
	return nil
}

func (a *App) GetPaymentHistory() ([]PaymentRecord, error) {
	return []PaymentRecord{}, nil
}

func (a *App) SavePaymentMethod(last4 string, brand string, expiry string) error {
	return nil
}

func (a *App) GetPaymentMethod() (*PaymentMethod, error) {
	return nil, nil
}
