package xray

import (
	"bytes"
	"crypto/tls"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/http/cookiejar"
	"net/url"
	"strings"
)

// Client communicates with 3X-UI panel API.
type Client struct {
	BaseURL    string
	Username   string
	Password   string
	httpClient *http.Client
	loggedIn   bool
}

type InboundClient struct {
	ID    string `json:"id"`
	Email string `json:"email"`
	Flow  string `json:"flow"`
}

type InboundInfo struct {
	Id             int             `json:"id"`
	Up             int64           `json:"up"`
	Down           int64           `json:"down"`
	Total          int64           `json:"total"`
	Remark         string          `json:"remark"`
	Enable         bool            `json:"enable"`
	ExpiryTime     int64           `json:"expiryTime"`
	ClientStats    []interface{}   `json:"clientStats"`
	Listen         string          `json:"listen"`
	Port           int             `json:"port"`
	Protocol       string          `json:"protocol"`
	Settings       json.RawMessage `json:"settings"`
	StreamSettings json.RawMessage `json:"streamSettings"`
	Tag            string          `json:"tag"`
	Sniffing       json.RawMessage `json:"sniffing"`
}

type VLESSConfig struct {
	UUID        string
	Host        string
	Port        int
	Security    string
	Flow        string
	SNI         string
	Fingerprint string
	PublicKey   string
	ShortID     string
	SpiderX     string
	Encryption  string
	PQV         string
}

// NewClient creates a 3X-UI API client.
func NewClient(baseURL, username, password string) *Client {
	jar, _ := cookiejar.New(nil)
	tr := &http.Transport{
		TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
	}
	return &Client{
		BaseURL:  strings.TrimRight(baseURL, "/"),
		Username: username,
		Password: password,
		httpClient: &http.Client{
			Jar:       jar,
			Transport: tr,
		},
	}
}

// Login authenticates with the 3X-UI panel.
func (c *Client) Login() error {
	payload := map[string]string{
		"username": c.Username,
		"password": c.Password,
	}
	data, _ := json.Marshal(payload)

	resp, err := c.httpClient.Post(c.BaseURL+"/login", "application/json", bytes.NewBuffer(data))
	if err != nil {
		return fmt.Errorf("login request failed: %w", err)
	}
	body, _ := io.ReadAll(resp.Body)

	var result map[string]interface{}
	if err := json.Unmarshal(body, &result); err != nil {
		// Maybe it returned a raw boolean or number
		if string(body) == "true" || string(body) == "1" || string(body) == "{\"success\":true}" {
			c.loggedIn = true
			return nil
		}
		return fmt.Errorf("failed to parse login response. Raw body: %s", string(body))
	}

	successRaw, exists := result["success"]
	if !exists || !isSuccess(successRaw) {
		msg, _ := result["msg"].(string)
		return fmt.Errorf("login failed: %s (Raw body: %s)", msg, string(body))
	}

	c.loggedIn = true
	return nil
}

// ensureLoggedIn performs login if not already authenticated.
func (c *Client) ensureLoggedIn() error {
	if !c.loggedIn {
		return c.Login()
	}
	return nil
}

// GetInbound returns info about a specific inbound by ID.
func (c *Client) GetInbound(inboundID int) (*InboundInfo, error) {
	if err := c.ensureLoggedIn(); err != nil {
		return nil, err
	}

	resp, err := c.httpClient.Get(fmt.Sprintf("%s/panel/api/inbounds/get/%d", c.BaseURL, inboundID))
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	var result struct {
		Success interface{} `json:"success"`
		Obj     InboundInfo `json:"obj"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&result); err != nil {
		return nil, err
	}
	if !isSuccess(result.Success) {
		return nil, fmt.Errorf("failed to get inbound %d", inboundID)
	}
	return &result.Obj, nil
}

// AddClient adds a new VLESS client to an inbound.
func (c *Client) AddClient(inboundID int, clientUUID, email string) error {
	if err := c.ensureLoggedIn(); err != nil {
		return err
	}

	client := InboundClient{
		ID:    clientUUID,
		Email: email,
		Flow:  "xtls-rprx-vision",
	}
	clientsJSON, _ := json.Marshal([]InboundClient{client})

	payload := map[string]interface{}{
		"id":       inboundID,
		"settings": fmt.Sprintf(`{"clients":%s}`, string(clientsJSON)),
	}
	data, _ := json.Marshal(payload)

	resp, err := c.httpClient.Post(
		fmt.Sprintf("%s/panel/api/inbounds/addClient", c.BaseURL),
		"application/json",
		bytes.NewBuffer(data),
	)
	if err != nil {
		return fmt.Errorf("add client request failed: %w", err)
	}
	defer resp.Body.Close()

	return c.checkResponse(resp)
}

// RemoveClient removes a client from an inbound by UUID.
func (c *Client) RemoveClient(inboundID int, clientUUID string) error {
	if err := c.ensureLoggedIn(); err != nil {
		return err
	}

	req, err := http.NewRequest(
		"POST",
		fmt.Sprintf("%s/panel/api/inbounds/%d/delClient/%s", c.BaseURL, inboundID, clientUUID),
		nil,
	)
	if err != nil {
		return err
	}

	resp, err := c.httpClient.Do(req)
	if err != nil {
		return fmt.Errorf("remove client request failed: %w", err)
	}
	defer resp.Body.Close()

	return c.checkResponse(resp)
}

// GetClients returns all clients for an inbound.
func (c *Client) GetClients(inboundID int) ([]InboundClient, error) {
	inbound, err := c.GetInbound(inboundID)
	if err != nil {
		return nil, err
	}

	var settings struct {
		Clients []InboundClient `json:"clients"`
	}

	// Try unmarshalling Settings as object first
	if err := json.Unmarshal(inbound.Settings, &settings); err != nil {
		// If that fails, try unmarshalling as string (double-encoded JSON)
		var settingsStr string
		if errStr := json.Unmarshal(inbound.Settings, &settingsStr); errStr == nil {
			if errObject := json.Unmarshal([]byte(settingsStr), &settings); errObject != nil {
				return nil, fmt.Errorf("failed to parse double-encoded inbound settings: %w", errObject)
			}
		} else {
			return nil, fmt.Errorf("failed to parse inbound settings: %w", err)
		}
	}
	return settings.Clients, nil
}

// BuildVLESSURI constructs a vless:// URI from configuration.
func BuildVLESSURI(cfg VLESSConfig) string {
	params := url.Values{}
	params.Set("type", "tcp")
	params.Set("security", cfg.Security)
	params.Set("flow", cfg.Flow)

	if cfg.Security == "reality" {
		params.Set("sni", cfg.SNI)
		params.Set("fp", cfg.Fingerprint)
		params.Set("pbk", cfg.PublicKey)
		if cfg.ShortID != "" {
			params.Set("sid", cfg.ShortID)
		}
		if cfg.SpiderX != "" {
			params.Set("spx", cfg.SpiderX)
		}
		if cfg.Encryption != "" && cfg.Encryption != "none" {
			params.Set("encryption", cfg.Encryption)
		}
		if cfg.PQV != "" {
			params.Set("pqv", cfg.PQV)
		}
	}

	return fmt.Sprintf("vless://%s@%s:%d?%s#DrFrakeVPN",
		cfg.UUID, cfg.Host, cfg.Port, params.Encode())
}

func (c *Client) checkResponse(resp *http.Response) error {
	body, _ := io.ReadAll(resp.Body)

	if resp.StatusCode >= 400 {
		return fmt.Errorf("3x-ui api error %d: %s", resp.StatusCode, string(body))
	}

	var result map[string]interface{}
	if err := json.Unmarshal(body, &result); err != nil {
		// fallback for raw success
		if string(body) == "true" || string(body) == "1" {
			return nil
		}
		return fmt.Errorf("3x-ui non-json response: %s", string(body))
	}

	successRaw, exists := result["success"]
	if exists && !isSuccess(successRaw) {
		msg, _ := result["msg"].(string)
		return fmt.Errorf("3x-ui error: %s (Raw body: %s)", msg, string(body))
	}
	return nil
}

// isSuccess checks whether the API returned success=true or success=1
func isSuccess(val interface{}) bool {
	switch v := val.(type) {
	case bool:
		return v
	case float64:
		return v == 1
	case int:
		return v == 1
	default:
		return false
	}
}
