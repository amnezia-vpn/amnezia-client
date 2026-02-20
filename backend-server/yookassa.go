package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"io"
	"net/http"

	"github.com/google/uuid"
)

type Amount struct {
	Value    string `json:"value"`
	Currency string `json:"currency"`
}

type Confirmation struct {
	Type            string `json:"type"`
	ReturnURL       string `json:"return_url,omitempty"`
	ConfirmationURL string `json:"confirmation_url,omitempty"`
}

type PaymentMetadata struct {
	UserID string `json:"user_id"`
	Tier   string `json:"tier"`
}

type PaymentRequest struct {
	Amount       Amount          `json:"amount"`
	Capture      bool            `json:"capture"`
	Confirmation Confirmation    `json:"confirmation"`
	Description  string          `json:"description"`
	Metadata     PaymentMetadata `json:"metadata"`
}

type PaymentResponse struct {
	ID           string          `json:"id"`
	Status       string          `json:"status"`
	Paid         bool            `json:"paid"`
	Amount       Amount          `json:"amount"`
	Confirmation Confirmation    `json:"confirmation"`
	Description  string          `json:"description"`
	Metadata     PaymentMetadata `json:"metadata"`
}

type YooKassaClient struct {
	ShopID    string
	SecretKey string
	BaseURL   string
}

func NewYooKassaClient(shopID, secretKey string) *YooKassaClient {
	return &YooKassaClient{
		ShopID:    shopID,
		SecretKey: secretKey,
		BaseURL:   "https://api.yookassa.ru/v3",
	}
}

func (c *YooKassaClient) CreatePayment(amount string, description string, userID string, tier string, returnURL string) (*PaymentResponse, error) {
	reqBody := PaymentRequest{
		Amount: Amount{
			Value:    amount,
			Currency: "RUB",
		},
		Capture: true,
		Confirmation: Confirmation{
			Type:      "redirect",
			ReturnURL: returnURL,
		},
		Description: description,
		Metadata: PaymentMetadata{
			UserID: userID,
			Tier:   tier,
		},
	}

	jsonBody, err := json.Marshal(reqBody)
	if err != nil {
		return nil, err
	}

	idempotenceKey := uuid.New().String()

	req, err := http.NewRequest("POST", c.BaseURL+"/payments", bytes.NewBuffer(jsonBody))
	if err != nil {
		return nil, err
	}

	c.setHeaders(req, idempotenceKey)

	return c.do(req)
}

func (c *YooKassaClient) GetPayment(paymentID string) (*PaymentResponse, error) {
	req, err := http.NewRequest("GET", c.BaseURL+"/payments/"+paymentID, nil)
	if err != nil {
		return nil, err
	}

	c.setHeaders(req, "")

	return c.do(req)
}

func (c *YooKassaClient) setHeaders(req *http.Request, idempotenceKey string) {
	auth := base64.StdEncoding.EncodeToString([]byte(c.ShopID + ":" + c.SecretKey))
	req.Header.Set("Authorization", "Basic "+auth)
	req.Header.Set("Content-Type", "application/json")
	if idempotenceKey != "" {
		req.Header.Set("Idempotence-Key", idempotenceKey)
	}
}

func (c *YooKassaClient) do(req *http.Request) (*PaymentResponse, error) {
	// Use a client that bypasses system proxy to avoid
	// "http: server gave HTTP response to HTTPS client" errors
	// when the VPN app has set a local HTTP proxy
	client := &http.Client{
		Transport: &http.Transport{
			Proxy: nil, // Explicitly bypass system proxy
		},
	}
	resp, err := client.Do(req)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()

	body, _ := io.ReadAll(resp.Body)

	if resp.StatusCode >= 400 {
		return nil, fmt.Errorf("yookassa api error: %s - %s", resp.Status, string(body))
	}

	var paymentResp PaymentResponse
	if err := json.Unmarshal(body, &paymentResp); err != nil {
		return nil, fmt.Errorf("failed to unmarshal response: %w", err)
	}

	return &paymentResp, nil
}
