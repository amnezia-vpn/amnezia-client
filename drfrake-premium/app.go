package main

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"log"
	"os"
	"path/filepath"
	"strings"

	"golang.getoutline.org/sdk/network"
	"golang.getoutline.org/sdk/network/lwip2transport"
	"golang.getoutline.org/sdk/x/configurl"
)

type Session struct {
	Token string `json:"token"`
	Email string `json:"email"`
	Plan  string `json:"plan"`
}

// App struct
type App struct {
	ctx          context.Context
	tunDevice    *WindowsTUN
	lwipDevice   network.IPDevice
	isConnected  bool
	activeConfig string
	subDB        *SubscriptionDB
	currentUser  *User
	config       *Config
	apiClient    *APIClient
	authToken    string
	xrayManager  *XrayManager
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

	// Initialize SQLite database (still used for local subscription/payment data)
	dataDir, err := os.UserConfigDir()
	if err != nil {
		dataDir = "."
	}
	dbDir := filepath.Join(dataDir, "DrFrakeVPN")
	os.MkdirAll(dbDir, 0755)
	dbPath := filepath.Join(dbDir, "drfrake.db")

	a.subDB, err = NewSubscriptionDB(dbPath)
	if err != nil {
		log.Fatalf("Failed to initialize database: %v", err)
	}
	log.Printf("Database initialized at %s\n", dbPath)

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
	if a.subDB != nil {
		a.subDB.Close()
	}
}

// --- Auth Methods ---

func (a *App) Register(email string, password string) (*User, error) {
	log.Printf("[App] Registering user %s using Backend URL: %s", email, a.apiClient.BaseURL)
	authResp, err := a.apiClient.Register(email, password)
	if err != nil {
		return nil, err
	}

	// Also register locally for subscription tracking
	a.subDB.Register(email, password)

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
	// Try backend API first
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
		log.Printf("[Servers] API failed, falling back to local: %v", err)
	}

	// Fallback to local servers.json
	configs, err := LoadServers()
	if err != nil {
		return []Server{}
	}

	var servers []Server
	for _, c := range configs {
		servers = append(servers, Server{
			ID:        c.ID,
			Country:   c.Country,
			Flag:      c.Flag,
			Config:    c.Config,
			IsPremium: c.IsPremium,
			Latency:   50 + len(c.City),
		})
	}
	return servers
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
			sub, err := a.subDB.GetSubscription(a.currentUser.ID)
			if err != nil {
				return fmt.Errorf("failed to check subscription: %w", err)
			}
			if sub.Plan == PlanFreeType || sub.Status == StatusExpired {
				return fmt.Errorf("premium subscription required for this server")
			}
		}
	}

	log.Printf("[VPN] Connecting with config: %s", config)

	// Detect protocol and prepare config for Outline SDK
	var serverHost string
	var dialerConfig string

	if strings.HasPrefix(config, "vless://") {
		// VLESS: start xray-core subprocess, use SOCKS5 bridge
		log.Printf("[VPN] Detected VLESS protocol, starting xray-core...")

		// Parse VLESS URI to get server host for routing
		vlessParams, err := ParseVLESSURI(config)
		if err != nil {
			return fmt.Errorf("failed to parse VLESS config: %w", err)
		}
		serverHost = vlessParams.Host

		// Start xray-core
		if a.xrayManager == nil {
			a.xrayManager = NewXrayManager()
		}
		if err := a.xrayManager.Start(config); err != nil {
			return fmt.Errorf("failed to start xray-core: %w", err)
		}

		// Use SOCKS5 proxy as the dialer config
		dialerConfig = a.xrayManager.GetSOCKS5Config()
		log.Printf("[VPN] Using SOCKS5 bridge: %s", dialerConfig)
	} else {
		// Shadowsocks or other protocol supported by Outline SDK
		dialerConfig = config
		if cfg, err := configurl.ParseConfig(config); err == nil {
			serverHost = cfg.URL.Hostname()
		}
	}
	// 1. Create Dialers
	providers := configurl.NewDefaultProviders()
	sd, err := providers.NewStreamDialer(context.Background(), dialerConfig)
	if err != nil {
		a.stopXray() // Clean up on failure
		return fmt.Errorf("failed to create stream dialer: %w", err)
	}
	pl, err := providers.NewPacketListener(context.Background(), dialerConfig)
	if err != nil {
		a.stopXray()
		return fmt.Errorf("failed to create packet listener: %w", err)
	}
	pp, err := network.NewPacketProxyFromPacketListener(pl)
	if err != nil {
		a.stopXray()
		return fmt.Errorf("failed to create packet proxy: %w", err)
	}

	// 2. Create & Configure TUN
	tun, err := NewWindowsTUN()
	if err != nil {
		a.stopXray()
		return fmt.Errorf("failed to create TUN device: %w", err)
	}
	// Use a fixed IP for now. Ideally should be configurable or determined by server.
	// But Outline usually doesn't push IP. We use a private IP.
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
		a.stopXray()
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

func (a *App) IsConnected() bool {
	return a.isConnected
}

// --- Subscription Methods (exposed to React) ---

func (a *App) GetSubscription() (*Subscription, error) {
	if a.currentUser == nil {
		return nil, fmt.Errorf("not logged in")
	}
	return a.subDB.GetSubscription(a.currentUser.ID)
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

	// If payment succeeded, upgrade local subscription DB too
	if status == "succeeded" && plan != "" {
		a.subDB.UpgradePlan(a.currentUser.ID, PlanType(plan))
		log.Printf("[Payment] Upgraded user %s to plan: %s", a.currentUser.Email, plan)
	}

	return status, nil
}

func (a *App) CancelAutoRenew() error {
	if a.currentUser == nil {
		return fmt.Errorf("not logged in")
	}
	return a.subDB.CancelAutoRenew(a.currentUser.ID)
}

func (a *App) EnableAutoRenew() error {
	if a.currentUser == nil {
		return fmt.Errorf("not logged in")
	}
	return a.subDB.EnableAutoRenew(a.currentUser.ID)
}

func (a *App) GetPaymentHistory() ([]PaymentRecord, error) {
	if a.currentUser == nil {
		return nil, fmt.Errorf("not logged in")
	}
	return a.subDB.GetPaymentHistory(a.currentUser.ID)
}

func (a *App) SavePaymentMethod(last4 string, brand string, expiry string) error {
	return nil // Deprecated, handled by YooKassa
}

func (a *App) GetPaymentMethod() (*PaymentMethod, error) {
	if a.currentUser == nil {
		return nil, fmt.Errorf("not logged in")
	}
	return a.subDB.GetPaymentMethod(a.currentUser.ID)
}
