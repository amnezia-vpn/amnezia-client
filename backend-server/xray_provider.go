package main

import (
	"encoding/json"
	"fmt"
	"log"

	"drfrake-backend/xray"

	"github.com/google/uuid"
)

// XrayProvider implements VPNProvider using 3X-UI panel API.
type XrayProvider struct {
	client     *xray.Client
	inboundID  int
	serverHost string // Public IP/hostname of the VPN server
	settings   XrayServerSettings
}

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
}

// NewXrayProvider creates a provider backed by a 3X-UI panel.
func NewXrayProvider(panelURL, username, password string, inboundID int, serverHost string, settingsJSON string) *XrayProvider {
	var settings XrayServerSettings
	if err := json.Unmarshal([]byte(settingsJSON), &settings); err != nil {
		log.Printf("Warning: failed to parse xray settings: %v", err)
		settings = XrayServerSettings{
			Port:        443,
			Flow:        "xtls-rprx-vision",
			Security:    "reality",
			SNI:         "google.com",
			Fingerprint: "chrome",
		}
	}

	return &XrayProvider{
		client:     xray.NewClient(panelURL, username, password),
		inboundID:  inboundID,
		serverHost: serverHost,
		settings:   settings,
	}
}

func (p *XrayProvider) CreateKey(userID string) (string, string, error) {
	email := fmt.Sprintf("user-%s", userID)

	// Check if user already exists to prevent duplicates
	clients, err := p.client.GetClients(p.inboundID)
	if err == nil {
		log.Printf("DEBUG: Found %d clients in inbound %d", len(clients), p.inboundID)
		for _, c := range clients {
			log.Printf("DEBUG: Client in Xray: ID=%s, Email=%s", c.ID, c.Email)
			if c.Email == email {
				log.Printf("User %s already exists in Xray, reusing key", userID)
				return c.ID, p.buildVLESSURI(c.ID), nil
			}
		}
	} else {
		log.Printf("Warning: failed to list clients: %v", err)
	}

	clientUUID := uuid.New().String()
	if err := p.client.AddClient(p.inboundID, clientUUID, email); err != nil {
		return "", "", fmt.Errorf("failed to create xray client: %w", err)
	}

	return clientUUID, p.buildVLESSURI(clientUUID), nil
}

func (p *XrayProvider) DeleteKey(keyID string) error {
	return p.client.RemoveClient(p.inboundID, keyID)
}

func (p *XrayProvider) GetKeys() ([]VPNKey, error) {
	clients, err := p.client.GetClients(p.inboundID)
	if err != nil {
		return nil, err
	}

	var keys []VPNKey
	for _, c := range clients {
		keys = append(keys, VPNKey{
			ID:        c.ID,
			Name:      c.Email,
			AccessURL: p.buildVLESSURI(c.ID),
		})
	}
	return keys, nil
}

func (p *XrayProvider) SetName(keyID string, name string) error {
	// 3X-UI uses email as identifier; name change not easily supported via API
	// This is a no-op for now
	return nil
}

func (p *XrayProvider) buildVLESSURI(uuid string) string {
	return xray.BuildVLESSURI(xray.VLESSConfig{
		UUID:        uuid,
		Host:        p.serverHost,
		Port:        p.settings.Port,
		Flow:        p.settings.Flow,
		Security:    p.settings.Security,
		SNI:         p.settings.SNI,
		Fingerprint: p.settings.Fingerprint,
		PublicKey:   p.settings.PublicKey,
		ShortID:     p.settings.ShortID,
		SpiderX:     p.settings.SpiderX,
	})
}
