// Plaintext mock for AmneziaVPN client (CMake AMNEZIA_LOCAL_GATEWAY=ON + localhost DEV_AGW_ENDPOINT).
// No RSA/AES — POST JSON is the same object the client sends inside api_payload when encrypted.
package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"io"
	"log"
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
)

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
				"is_available":       true,
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

func main() {
	http.HandleFunc("/v1/services", handleServices)
	http.HandleFunc("/v1/config", handleConfig)
	log.Println("plaintext mock listening on :8080  POST /v1/services  POST /v1/config")
	log.Fatal(http.ListenAndServe(":8080", nil))
}
