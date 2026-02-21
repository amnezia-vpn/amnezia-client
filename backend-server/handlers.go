package main

import (
	"database/sql"
	"encoding/json"
	"log"
	"net/http"

	"github.com/google/uuid"
)

type RegisterRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

type LoginRequest struct {
	Email    string `json:"email"`
	Password string `json:"password"`
}

type AuthResponse struct {
	Token string `json:"token"`
	User  User   `json:"user"`
}

type User struct {
	ID    string `json:"id"`
	Email string `json:"email"`
	Plan  string `json:"plan"`
}

func (s *Server) handleRegister(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Method not allowed", 405)
		return
	}
	var req RegisterRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", 400)
		return
	}

	// TODO: Hash password! For demo, plaintext (bad practice but simple for now)
	id := uuid.New().String()
	_, err := s.DB.Exec("INSERT INTO users (id, email, password, plan) VALUES (?, ?, ?, ?)", id, req.Email, req.Password, "free")
	if err != nil {
		http.Error(w, "User exists or error", 500)
		return
	}

	json.NewEncoder(w).Encode(map[string]string{"status": "ok", "id": id})
}

func (s *Server) handleLogin(w http.ResponseWriter, r *http.Request) {
	var req LoginRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", 400)
		return
	}

	var user User
	var pwd string
	err := s.DB.QueryRow("SELECT id, email, password, plan FROM users WHERE email = ?", req.Email).Scan(&user.ID, &user.Email, &pwd, &user.Plan)
	if err != nil || pwd != req.Password {
		http.Error(w, "Invalid credentials", 401)
		return
	}

	// Simple token = UserID for demo. Real world: JWT.
	resp := AuthResponse{
		Token: user.ID,
		User:  user,
	}
	json.NewEncoder(w).Encode(resp)
}

func (s *Server) handleGetServers(w http.ResponseWriter, r *http.Request) {
	token := r.Header.Get("Authorization")
	if token == "" {
		http.Error(w, "Unauthorized", 401)
		return
	}

	// Check if user exists and get plan
	var plan string
	err := s.DB.QueryRow("SELECT plan FROM users WHERE id = ?", token).Scan(&plan)
	if err != nil {
		http.Error(w, "Unauthorized", 401)
		return
	}

	// Get all active servers
	rows, err := s.DB.Query(`SELECT id, country, city, flag, is_premium,
		type, server_host, ssh_user, ssh_pass, settings
		FROM servers`)
	if err != nil {
		http.Error(w, "Database error", 500)
		return
	}
	defer rows.Close()

	var servers []map[string]interface{}

	for rows.Next() {
		var srvID, country, city, flag string
		var isPremium bool
		var srvType, serverHost, sshUser, sshPass, settings string

		if err := rows.Scan(&srvID, &country, &city, &flag, &isPremium,
			&srvType, &serverHost, &sshUser, &sshPass, &settings); err != nil {
			log.Printf("Error scanning server row: %v", err)
			continue
		}

		// Check/Create Access Key
		var keyID, accessURL string
		err := s.DB.QueryRow("SELECT key_id, access_url FROM access_keys WHERE user_id = ? AND server_id = ?", token, srvID).Scan(&keyID, &accessURL)

		if err == sql.ErrNoRows {
			// Initialize the Amnezia provider
			provider := NewAmneziaProvider(serverHost, sshUser, sshPass, settings)

			// Check if key already exists (idempotency)
			var foundKeyID, foundKeyURL string
			keys, listErr := provider.GetKeys()
			if listErr == nil {
				for _, k := range keys {
					if k.Name == "user-"+token {
						foundKeyID = k.ID
						foundKeyURL = k.AccessURL
						break
					}
				}
			}

			// If not found, create new key
			if foundKeyID == "" {
				newID, newURL, createErr := provider.CreateKey(token)
				if createErr != nil {
					log.Printf("Failed to create key for user %s on server %s (%s): %v", token, srvID, srvType, createErr)
					continue
				}
				foundKeyID = newID
				foundKeyURL = newURL
			}

			// Save to DB
			_, dbErr := s.DB.Exec("INSERT INTO access_keys (user_id, server_id, key_id, access_url) VALUES (?, ?, ?, ?)",
				token, srvID, foundKeyID, foundKeyURL)
			if dbErr != nil {
				log.Printf("DB Insert Warning (Key might exist): %v", dbErr)
			}

			accessURL = foundKeyURL
		} else if err != nil {
			log.Printf("DB Error fetching key: %v", err)
			continue
		}

		// Add to response
		servers = append(servers, map[string]interface{}{
			"id":        srvID,
			"country":   country,
			"city":      city,
			"flag":      flag,
			"config":    accessURL,
			"isPremium": isPremium,
			"type":      srvType,
		})
	}

	if servers == nil {
		servers = []map[string]interface{}{}
	}
	json.NewEncoder(w).Encode(servers)
}

func (s *Server) handleAdminAddServer(w http.ResponseWriter, r *http.Request) {
	// Simple validation - strictly for local/trusted usage now
	var req struct {
		Country    string `json:"country"`
		City       string `json:"city"`
		Flag       string `json:"flag"`
		IsPremium  bool   `json:"is_premium"`
		Type       string `json:"type"` // "amnezia" (default)
		ServerHost string `json:"server_host"`
		SSHUser    string `json:"ssh_user"`
		SSHPass    string `json:"ssh_pass"`
		Settings   string `json:"settings"` // JSON string with Reality params
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", 400)
		return
	}

	// Defaults
	if req.Type == "" {
		req.Type = "amnezia"
	}
	if req.Settings == "" {
		req.Settings = "{}"
	}

	id := uuid.New().String()
	_, err := s.DB.Exec(`INSERT INTO servers
		(id, country, city, flag, is_premium, type, server_host,
		 ssh_user, ssh_pass, settings)
		VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)`,
		id, req.Country, req.City, req.Flag, req.IsPremium,
		req.Type, req.ServerHost, req.SSHUser, req.SSHPass, req.Settings)

	if err != nil {
		http.Error(w, "Database error: "+err.Error(), 500)
		return
	}

	json.NewEncoder(w).Encode(map[string]string{"status": "ok", "id": id, "type": req.Type})
}

func (s *Server) handleInitPayment(w http.ResponseWriter, r *http.Request) {
	if r.Method != "POST" {
		http.Error(w, "Method not allowed", 405)
		return
	}

	token := r.Header.Get("Authorization")
	if token == "" {
		http.Error(w, "Unauthorized", 401)
		return
	}

	// Verify user
	var plan string
	err := s.DB.QueryRow("SELECT plan FROM users WHERE id = ?", token).Scan(&plan)
	if err != nil {
		http.Error(w, "Unauthorized", 401)
		return
	}

	var req struct {
		Plan string `json:"plan"`
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Bad request", 400)
		return
	}

	// Calculate amount based on plan
	var amount, desc string
	switch req.Plan {
	case "monthly":
		amount = "299.00"
		desc = "Dr. Frake VPN — Premium Monthly"
	case "yearly":
		amount = "2990.00"
		desc = "Dr. Frake VPN — Premium Yearly"
	default:
		http.Error(w, "Invalid plan", 400)
		return
	}

	returnURL := s.Cfg.YookassaReturnURL
	if returnURL == "" {
		returnURL = "https://google.com"
	}

	// Call YooKassa API (server-side only!)
	payResp, err := s.YooKassa.CreatePayment(amount, desc, token, req.Plan, returnURL)
	if err != nil {
		http.Error(w, "Payment error: "+err.Error(), 500)
		return
	}

	// Store payment in DB
	s.DB.Exec("INSERT INTO payments (id, user_id, yookassa_id, amount, plan, status) VALUES (?, ?, ?, ?, ?, ?)",
		payResp.ID, token, payResp.ID, amount, req.Plan, payResp.Status)

	// Return confirmation URL to client
	json.NewEncoder(w).Encode(map[string]string{
		"id":               payResp.ID,
		"status":           payResp.Status,
		"confirmation_url": payResp.Confirmation.ConfirmationURL,
	})
}

func (s *Server) handleCheckPayment(w http.ResponseWriter, r *http.Request) {
	token := r.Header.Get("Authorization")
	if token == "" {
		http.Error(w, "Unauthorized", 401)
		return
	}

	paymentID := r.URL.Query().Get("id")
	if paymentID == "" {
		http.Error(w, "Missing payment id", 400)
		return
	}

	// Check payment status from YooKassa
	payResp, err := s.YooKassa.GetPayment(paymentID)
	if err != nil {
		http.Error(w, "Error checking payment: "+err.Error(), 500)
		return
	}

	// If payment succeeded, upgrade user
	if payResp.Status == "succeeded" {
		tier := payResp.Metadata.Tier
		if tier == "" {
			tier = "monthly"
		}
		s.DB.Exec("UPDATE users SET plan = ? WHERE id = ?", tier, token)
		s.DB.Exec("UPDATE payments SET status = ? WHERE yookassa_id = ?", "succeeded", paymentID)
	}

	json.NewEncoder(w).Encode(map[string]string{
		"status": payResp.Status,
		"plan":   payResp.Metadata.Tier,
	})
}

func (s *Server) handleWebhook(w http.ResponseWriter, r *http.Request) {
	// Handle YooKassa webhook — updates payment/user status
	w.WriteHeader(200)
}
