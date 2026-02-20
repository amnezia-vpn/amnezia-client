package main

// VPNProvider is an interface for managing VPN access keys across different backends.
type VPNProvider interface {
	// CreateKey creates a new access key for a user. Returns key ID and access config string.
	// For Outline: config is "ss://..." URI
	// For Xray: config is "vless://..." URI
	CreateKey(userID string) (keyID string, accessConfig string, err error)

	// DeleteKey removes an access key.
	DeleteKey(keyID string) error

	// GetKeys returns all access keys managed by this provider.
	GetKeys() ([]VPNKey, error)

	// SetName sets a human-readable name for a key (for tracking).
	SetName(keyID string, name string) error
}

// VPNKey represents an access key from any VPN provider.
type VPNKey struct {
	ID        string `json:"id"`
	Name      string `json:"name"`
	AccessURL string `json:"accessUrl"`
}

// ServerType identifies which VPN backend a server uses.
type ServerType string

const (
	ServerTypeXray ServerType = "xray"
)
