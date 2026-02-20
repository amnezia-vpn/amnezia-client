package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
)

// APIClient communicates with the Dr. Frake backend server
type APIClient struct {
	BaseURL string
	Token   string
}

func NewAPIClient(baseURL string) *APIClient {
	return &APIClient{BaseURL: baseURL}
}

// --- Auth ---

type APIAuthResponse struct {
	Token string  `json:"token"`
	User  APIUser `json:"user"`
}

type APIUser struct {
	ID    string `json:"id"`
	Email string `json:"email"`
	Plan  string `json:"plan"`
}

type APIServer struct {
	ID        string `json:"id"`
	Country   string `json:"country"`
	Flag      string `json:"flag"`
	City      string `json:"city"`
	Config    string `json:"config"`
	IsPremium bool   `json:"isPremium"`
	Type      string `json:"type"` // "outline" or "xray"
}

func (c *APIClient) Register(email, password string) (*APIAuthResponse, error) {
	payload := map[string]string{"email": email, "password": password}
	data, _ := json.Marshal(payload)

	resp, err := http.Post(c.BaseURL+"/register", "application/json", bytes.NewBuffer(data))
	if err != nil {
		return nil, fmt.Errorf("connection error: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		body, _ := io.ReadAll(resp.Body)
		return nil, fmt.Errorf("registration failed: %s", string(body))
	}

	// Register returns {"status":"ok","id":"..."}, need to login after
	// Do auto-login
	return c.Login(email, password)
}

func (c *APIClient) Login(email, password string) (*APIAuthResponse, error) {
	payload := map[string]string{"email": email, "password": password}
	data, _ := json.Marshal(payload)

	resp, err := http.Post(c.BaseURL+"/login", "application/json", bytes.NewBuffer(data))
	if err != nil {
		return nil, fmt.Errorf("connection error: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		body, _ := io.ReadAll(resp.Body)
		return nil, fmt.Errorf("login failed: %s", string(body))
	}

	var authResp APIAuthResponse
	if err := json.NewDecoder(resp.Body).Decode(&authResp); err != nil {
		return nil, fmt.Errorf("failed to parse response: %w", err)
	}

	c.Token = authResp.Token
	return &authResp, nil
}

// --- Servers ---

func (c *APIClient) GetServers() ([]APIServer, error) {
	req, err := http.NewRequest("GET", c.BaseURL+"/servers", nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Authorization", c.Token)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("connection error: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode == 401 {
		return nil, fmt.Errorf("unauthorized: please login again")
	}
	if resp.StatusCode != 200 {
		return nil, fmt.Errorf("server error: %d", resp.StatusCode)
	}

	var servers []APIServer
	if err := json.NewDecoder(resp.Body).Decode(&servers); err != nil {
		return nil, err
	}
	return servers, nil
}

// ValidateToken checks if a stored token is still valid by calling /servers
func (c *APIClient) ValidateToken(token string) (*APIUser, error) {
	c.Token = token
	req, err := http.NewRequest("GET", c.BaseURL+"/servers", nil)
	if err != nil {
		return nil, err
	}
	req.Header.Set("Authorization", token)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		return nil, fmt.Errorf("token invalid")
	}

	// Token is valid. We need to get user info.
	// For now, return a minimal user from the token (which is the user ID)
	return &APIUser{ID: token}, nil
}

// --- Payments (delegated to backend) ---

type APIPaymentResponse struct {
	ID              string `json:"id"`
	Status          string `json:"status"`
	ConfirmationURL string `json:"confirmation_url"`
}

func (c *APIClient) InitPayment(plan string) (*APIPaymentResponse, error) {
	payload := map[string]string{"plan": plan}
	data, _ := json.Marshal(payload)

	req, err := http.NewRequest("POST", c.BaseURL+"/payment/init", bytes.NewBuffer(data))
	if err != nil {
		return nil, err
	}
	req.Header.Set("Authorization", c.Token)
	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return nil, fmt.Errorf("connection error: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != 200 {
		body, _ := io.ReadAll(resp.Body)
		return nil, fmt.Errorf("payment init failed: %s", string(body))
	}

	var payResp APIPaymentResponse
	if err := json.NewDecoder(resp.Body).Decode(&payResp); err != nil {
		return nil, err
	}
	return &payResp, nil
}

func (c *APIClient) CheckPayment(paymentID string) (string, string, error) {
	req, err := http.NewRequest("GET", c.BaseURL+"/payment/check?id="+paymentID, nil)
	if err != nil {
		return "", "", err
	}
	req.Header.Set("Authorization", c.Token)

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		return "", "", fmt.Errorf("connection error: %w", err)
	}
	defer resp.Body.Close()

	var result struct {
		Status string `json:"status"`
		Plan   string `json:"plan"`
	}
	json.NewDecoder(resp.Body).Decode(&result)
	return result.Status, result.Plan, nil
}
