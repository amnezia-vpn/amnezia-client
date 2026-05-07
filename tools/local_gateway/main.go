// Plaintext mock for AmneziaVPN client (CMake AMNEZIA_LOCAL_GATEWAY=ON + localhost DEV_AGW_ENDPOINT).
// No RSA/AES — POST JSON is the same object the client sends inside api_payload when encrypted.
package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"io"
	"log"
	"net"
	"net/http"
	"sync"
	"time"

	"github.com/dchest/captcha"
)

func shortID(id string) string {
	if len(id) <= 10 {
		return id
	}
	return id[:10] + "…"
}

// Set to 5 to mimic "more than 5 requests per 24h". Set to 0 so the first amnezia-free request returns CAPTCHA (faster UI test).
const rateLimitExcessAfter = 0

var (
	mu       sync.Mutex
	requests = map[string][]time.Time{} // installation_uuid -> timestamps (sliding window simplified: count in session)
	sessions = map[string]*pairingSession{}
)

// Local dev only: gateway team agreed 120s for mock vs 30s production (task_1 docs).
const (
	pairingTTL        = 120 * time.Second
	longPollWaitLimit = 120 * time.Second
)

type generateQRRequest struct {
	QRUUID           string `json:"qr_uuid"`
	InstallationUUID string `json:"installation_uuid"`
	AppVersion       string `json:"app_version"`
	OSVersion        string `json:"os_version"`
}

type authData struct {
	APIKey string `json:"api_key"`
}

type scanQRRequest struct {
	QRUUID           string         `json:"qr_uuid"`
	Config           string         `json:"config"`
	ServiceInfo      map[string]any `json:"service_info"`
	SupportedProto   []string       `json:"supported_protocols"`
	AuthData         authData       `json:"auth_data"`
	InstallationUUID string         `json:"installation_uuid"`
	AppVersion       string         `json:"app_version"`
	OSVersion        string         `json:"os_version"`
}

type pairingResult struct {
	Config         string         `json:"config"`
	ServiceInfo    map[string]any `json:"service_info"`
	SupportedProto []string       `json:"supported_protocols"`
}

type pairingSession struct {
	QRUUID    string
	ExpiresAt time.Time
	Done       chan struct{}
	Result     *pairingResult
	Completed  bool
}

func writeJSON(w http.ResponseWriter, status int, body any) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(status)
	_ = json.NewEncoder(w).Encode(body)
}

func cleanupExpiredSessions(now time.Time) {
	for uuid, session := range sessions {
		if now.After(session.ExpiresAt) {
			delete(sessions, uuid)
		}
	}
}

func validateGenerateQRRequest(req generateQRRequest) bool {
	return req.QRUUID != "" && req.InstallationUUID != "" && req.AppVersion != "" && req.OSVersion != ""
}

func validateScanQRRequest(req scanQRRequest) bool {
	return req.QRUUID != "" &&
		req.Config != "" &&
		req.ServiceInfo != nil &&
		req.SupportedProto != nil &&
		req.AuthData.APIKey != "" &&
		req.InstallationUUID != "" &&
		req.AppVersion != "" &&
		req.OSVersion != ""
}

func pruneRequests(uuid string) {
	now := time.Now()
	cutoff := now.Add(-24 * time.Hour)
	var kept []time.Time
	for _, t := range requests[uuid] {
		if t.After(cutoff) {
			kept = append(kept, t)
		}
	}
	requests[uuid] = kept
}

func overLimit(uuid string) bool {
	pruneRequests(uuid)
	return len(requests[uuid]) > rateLimitExcessAfter
}

func handleGenerateQR(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}

	var req generateQRRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "json", http.StatusBadRequest)
		return
	}
	if !validateGenerateQRRequest(req) {
		writeJSON(w, http.StatusBadRequest, map[string]string{
			"message": "Bad Request. The payload is missing required fields or contains invalid values.",
		})
		return
	}

	session := &pairingSession{
		QRUUID:    req.QRUUID,
		ExpiresAt: time.Now().Add(pairingTTL),
		Done:      make(chan struct{}),
	}

	mu.Lock()
	cleanupExpiredSessions(time.Now())
	sessions[req.QRUUID] = session
	mu.Unlock()

	log.Printf("pairing REGISTERED uuid=%s ttl=%s", shortID(req.QRUUID), pairingTTL)

	timer := time.NewTimer(longPollWaitLimit)
	defer timer.Stop()

	select {
	case <-session.Done:
		mu.Lock()
		result := session.Result
		if sessions[req.QRUUID] == session {
			delete(sessions, req.QRUUID)
		}
		mu.Unlock()
		if result == nil {
			writeJSON(w, http.StatusInternalServerError, map[string]string{
				"message": "Internal Server Error: Pairing completed without payload.",
			})
			return
		}
		writeJSON(w, http.StatusOK, result)
	case <-timer.C:
		mu.Lock()
		if sessions[req.QRUUID] == session {
			delete(sessions, req.QRUUID)
		}
		mu.Unlock()
		writeJSON(w, http.StatusRequestTimeout, map[string]string{
			"message": "Request Timeout: No config received within the allowed time.",
		})
	}
}

func handleScanQR(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}

	var req scanQRRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "json", http.StatusBadRequest)
		return
	}
	if !validateScanQRRequest(req) {
		writeJSON(w, http.StatusBadRequest, map[string]string{
			"message": "Bad Request. The payload is missing required fields or contains invalid values.",
		})
		return
	}

	// Keep compatibility with current gateway behavior: key problems are mapped to 403.
	if req.AuthData.APIKey == "invalid" {
		writeJSON(w, http.StatusForbidden, map[string]string{
			"detail": "Forbidden: Invalid API key or unauthorized request.",
		})
		return
	}

	mu.Lock()
	cleanupExpiredSessions(time.Now())
	session, ok := sessions[req.QRUUID]
	if !ok || time.Now().After(session.ExpiresAt) {
		mu.Unlock()
		writeJSON(w, http.StatusNotFound, map[string]string{
			"message": "Not Found: QR session not found or expired.",
		})
		return
	}
	if session.Completed {
		mu.Unlock()
		writeJSON(w, http.StatusConflict, map[string]string{
			"message": "Conflict: Config already submitted for this QR session.",
		})
		return
	}

	session.Result = &pairingResult{
		Config:         req.Config,
		ServiceInfo:    req.ServiceInfo,
		SupportedProto: req.SupportedProto,
	}
	session.Completed = true
	close(session.Done)
	mu.Unlock()

	log.Printf("pairing COMPLETED uuid=%s config_len=%d", shortID(req.QRUUID), len(req.Config))
	writeJSON(w, http.StatusOK, map[string]string{"message": "OK"})
}

func handleServices(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	_, _ = io.Copy(io.Discard, r.Body)
	_ = r.Body.Close()

	// Minimal shape for ApiServicesModel::updateModel + importFreeFromGateway (service_protocol "awg").
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	_ = json.NewEncoder(w).Encode(map[string]any{
		"user_country_code": "ZZ",
		"services": []map[string]any{
			{
				"service_type":     "amnezia-free",
				"service_protocol": "awg",
				"service_info":     map[string]any{},
				"is_available":     true,
				"service_description": map[string]any{
					"service_name":     "Amnezia Free (mock)",
					"card_description": "Local plaintext mock",
					"description":      "For CAPTCHA UI test only",
				},
				"available_countries": []any{},
			},
		},
	})
}

func handleConfig(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}

	var body map[string]any
	if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
		http.Error(w, "json", http.StatusBadRequest)
		return
	}

	st, _ := body["service_type"].(string)
	if st != "amnezia-free" {
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)
		_ = json.NewEncoder(w).Encode(map[string]string{"message": "mock: only amnezia-free"})
		return
	}

	uuid, _ := body["installation_uuid"].(string)
	if uuid == "" {
		uuid = "anonymous"
	}

	captchaID, _ := body["captcha_id"].(string)
	solution, _ := body["captcha_solution"].(string)
	refresh, _ := body["refresh_captcha"].(bool)

	if refresh {
		var buf bytes.Buffer
		id := captcha.NewLen(6)
		_ = captcha.WriteImage(&buf, id, 240, 80)
		b64 := base64.StdEncoding.EncodeToString(buf.Bytes())

		log.Printf("captcha REFRESH id=%s uuid=%s", shortID(id), uuid)

		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusOK)

		_ = json.NewEncoder(w).Encode(map[string]string{
			"captcha_id":    id,
			"captcha_image": b64,
			"hint":          "Refreshed CAPTCHA",
		})
		return
	}

	if captchaID != "" && solution != "" {
		if captcha.VerifyString(captchaID, solution) {
			mu.Lock()
			requests[uuid] = nil
			mu.Unlock()
			log.Printf("captcha VERIFIED id=%s uuid=%s (dchest.VerifyString ok) -> HTTP 200", shortID(captchaID), uuid)
			// HTTP 200, no http_status:501 in body — client maps 501 to ApiUpdateRequestError ("update the app").
			w.Header().Set("Content-Type", "application/json")
			w.WriteHeader(http.StatusOK)
			_ = json.NewEncoder(w).Encode(map[string]any{
				"captcha_verified": true,
				"message":          "mock gateway: captcha ok — no vpn:// config in this mock (expect empty-config error in client)",
			})
			return
		}
		log.Printf("captcha REJECTED id=%s uuid=%s solution_len=%d (dchest.VerifyString failed) -> HTTP 402 invalid_captcha",
			shortID(captchaID), uuid, len(solution))
		var buf bytes.Buffer
		id := captcha.NewLen(6)
		_ = captcha.WriteImage(&buf, id, 240, 80)
		b64 := base64.StdEncoding.EncodeToString(buf.Bytes())
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusPaymentRequired)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error":         "invalid_captcha",
			"captcha_id":    id,
			"captcha_image": b64,
			"hint":          "Try again",
		})
		return
	}

	mu.Lock()
	requests[uuid] = append(requests[uuid], time.Now())
	limit := overLimit(uuid)
	mu.Unlock()

	if limit {
		var buf bytes.Buffer
		id := captcha.NewLen(6)
		_ = captcha.WriteImage(&buf, id, 240, 80)
		b64 := base64.StdEncoding.EncodeToString(buf.Bytes())
		log.Printf("captcha ISSUED id=%s uuid=%s (402 rate_limit_exceeded)", shortID(id), uuid)
		w.Header().Set("Content-Type", "application/json")
		w.WriteHeader(http.StatusPaymentRequired)
		_ = json.NewEncoder(w).Encode(map[string]string{
			"error":         "rate_limit_exceeded",
			"captcha_id":    id,
			"captcha_image": b64,
			"hint":          "Enter the digits from the image to continue",
		})
		return
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	_ = json.NewEncoder(w).Encode(map[string]string{
		"message": "mock: under rate limit, no config payload",
	})
}

// GET / — smoke test from a phone browser; avoids macOS oddities with IPv6 *:8080 + curl to own LAN IP.
func handleRoot(w http.ResponseWriter, r *http.Request) {
	if r.URL.Path != "/" {
		http.NotFound(w, r)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte("local_gateway plaintext mock\nPOST /api/v1/generate_qr, /api/v1/scan_qr, /v1/services, /v1/config\n"))
}

func main() {
	http.HandleFunc("/", handleRoot)
	http.HandleFunc("/v1/services", handleServices)
	http.HandleFunc("/v1/config", handleConfig)
	http.HandleFunc("/api/v1/generate_qr", handleGenerateQR)
	http.HandleFunc("/api/v1/scan_qr", handleScanQR)
	const addr = "0.0.0.0:8080"
	log.Printf("plaintext mock listening on tcp4 %s  GET /  POST /v1/services  POST /v1/config  POST /api/v1/generate_qr  POST /api/v1/scan_qr\n", addr)
	ln, err := net.Listen("tcp4", addr)
	if err != nil {
		log.Fatal(err)
	}
	log.Fatal(http.Serve(ln, nil))
}
