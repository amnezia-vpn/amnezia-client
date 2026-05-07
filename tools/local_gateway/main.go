// Plaintext mock for AmneziaVPN client (CMake AMNEZIA_QR_PAIRING_ALLOW; optional AMNEZIA_LAN_PLAINTEXT_GATEWAY for RFC1918 hosts).
// No RSA/AES — POST JSON is the same object the client sends inside api_payload when encrypted.
package main

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"
	"sort"
	"strings"
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

var (
	mu       sync.Mutex
	requests = map[string][]time.Time{} // installation_uuid -> timestamps (sliding window simplified: count in session)
	sessions = map[string]*pairingSession{}

	// Configured from flags / env in main().
	pairingSessionTTL    = 120 * time.Second
	longPollWaitLimit    = 120 * time.Second
	rateLimitExcessAfter = 0 // Set to 5 to mimic "more than 5 requests per 24h". 0 = first amnezia-free request may return CAPTCHA.
	// No trailing slash; used by POST /v1/updater_endpoint so remote clients (e.g. iOS) poll the Mac, not 127.0.0.1 on-device.
	publicUpdaterBaseURL string
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

func drainBody(r *http.Request) {
	_, _ = io.Copy(io.Discard, r.Body)
	_ = r.Body.Close()
}

// statusResponseWriter captures HTTP status for access-style logging.
type statusResponseWriter struct {
	http.ResponseWriter
	status  int
	written bool
}

func (w *statusResponseWriter) WriteHeader(code int) {
	if !w.written {
		w.status = code
		w.written = true
	}
	w.ResponseWriter.WriteHeader(code)
}

func (w *statusResponseWriter) Write(b []byte) (int, error) {
	if !w.written {
		w.status = http.StatusOK
		w.written = true
	}
	return w.ResponseWriter.Write(b)
}

// logReq logs every request with remote addr, UA, and final status (docs/local-gateway-mock.md).
func logReq(next http.HandlerFunc) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		srw := &statusResponseWriter{ResponseWriter: w, status: http.StatusOK}
		start := time.Now()
		log.Printf("REQ start remote=%s method=%s path=%s query=%s ua=%q x_client_request_id=%q content_type=%q content_length=%d",
			r.RemoteAddr, r.Method, r.URL.Path, r.URL.RawQuery, r.Header.Get("User-Agent"), r.Header.Get("X-Client-Request-ID"),
			r.Header.Get("Content-Type"), r.ContentLength)
		next(srw, r)
		log.Printf("REQ end remote=%s method=%s path=%s status=%d dur=%s",
			r.RemoteAddr, r.Method, r.URL.Path, srw.status, time.Since(start).Round(time.Millisecond))
	}
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
		ExpiresAt: time.Now().Add(pairingSessionTTL),
		Done:      make(chan struct{}),
	}

	mu.Lock()
	cleanupExpiredSessions(time.Now())
	sessions[req.QRUUID] = session
	mu.Unlock()

	log.Printf("pairing REGISTERED uuid=%s install=%s ttl=%s app=%s os=%s",
		shortID(req.QRUUID), shortID(req.InstallationUUID), pairingSessionTTL, req.AppVersion, req.OSVersion)

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

	log.Printf("pairing COMPLETED uuid=%s phone_install=%s config_len=%d proto_count=%d",
		shortID(req.QRUUID), shortID(req.InstallationUUID), len(req.Config), len(req.SupportedProto))
	writeJSON(w, http.StatusOK, map[string]string{"message": "OK"})
}

func handleServices(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)

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
	_, _ = w.Write([]byte("local_gateway plaintext mock — full path list: tools/local_gateway/README.md\n"))
}

// POST /v1/account_info — same path as SubscriptionController::getAccountInfo (ApiAccountInfoModel::updateModel).
func handleAccountInfo(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)

	// Keys match client/core/utils/constants/apiKeys.h (snake_case).
	endDate := time.Now().UTC().AddDate(1, 0, 0).Format(time.RFC3339)
	resp := map[string]any{
		"active_device_count":      1,
		"max_device_count":         5,
		"subscription_end_date":    endDate,
		"subscription_description": "Local mock (tools/local_gateway)",
		"is_renewal_available":     false,
		"supported_protocols":      []string{"awg", "vless"},
		"available_countries":      []any{},
		"issued_configs":           []any{},
		"support_info": map[string]any{
			"telegram":      "amnezia_support",
			"email":         "support@example.com",
			"billing_email": "billing@example.com",
			"website":       "https://amnezia.org",
			"website_name":  "Amnezia",
		},
	}
	writeJSON(w, http.StatusOK, resp)
}

// POST /v1/news — NewsController::fetchNews (empty list is fine).
func handleNews(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)
	writeJSON(w, http.StatusOK, map[string]any{"news": []any{}})
}

// POST /v1/renewal_link — SubscriptionController::getRenewalLink.
func handleRenewalLink(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)
	writeJSON(w, http.StatusOK, map[string]string{"renewal_url": "https://amnezia.org/"})
}

// POST /v1/updater_endpoint — UpdateController::fetchGatewayUrl, then GET {url}/VERSION.
func handleUpdaterEndpoint(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)
	log.Printf("updater_endpoint response url=%q", publicUpdaterBaseURL)
	writeJSON(w, http.StatusOK, map[string]string{"url": publicUpdaterBaseURL})
}

// POST /v1/revoke_config, /v1/revoke_native_config — success body ignored if error is NoError.
func handleRevokeNoop(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	drainBody(r)
	writeJSON(w, http.StatusOK, map[string]string{"message": "mock"})
}

func handleGetVersion(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
	_, _ = w.Write([]byte("0.0.1"))
}

func handleGetChangelog(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
}

func handleGetReleaseDate(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodGet {
		http.Error(w, "method", http.StatusMethodNotAllowed)
		return
	}
	w.Header().Set("Content-Type", "text/plain; charset=utf-8")
	w.WriteHeader(http.StatusOK)
}

func envOrDefault(key, def string) string {
	if v := strings.TrimSpace(os.Getenv(key)); v != "" {
		return v
	}
	return def
}

func cloneIPv4(ip net.IP) net.IP {
	x := make(net.IP, 4)
	copy(x, ip.To4())
	return x
}

// pickLANIPv4 returns a stable choice of non-loopback IPv4 for updater_endpoint / banners (prefers private ULA space).
func pickLANIPv4() net.IP {
	ifaces, err := net.Interfaces()
	if err != nil {
		log.Printf("net.Interfaces: %v", err)
		return nil
	}
	type cand struct {
		ip      net.IP
		private bool
		name    string
	}
	var cands []cand
	for _, iface := range ifaces {
		if iface.Flags&net.FlagUp == 0 {
			continue
		}
		if iface.Flags&net.FlagLoopback != 0 {
			continue
		}
		addrs, err := iface.Addrs()
		if err != nil {
			continue
		}
		for _, a := range addrs {
			ipNet, ok := a.(*net.IPNet)
			if !ok {
				continue
			}
			ip4 := ipNet.IP.To4()
			if ip4 == nil || ip4.IsLoopback() {
				continue
			}
			priv := ip4.IsPrivate() || ip4.IsLinkLocalUnicast()
			cands = append(cands, cand{ip: cloneIPv4(ip4), private: priv, name: iface.Name})
			log.Printf("iface candidate name=%s ip=%s private_or_linklocal=%v", iface.Name, ip4, priv)
		}
	}
	if len(cands) == 0 {
		return nil
	}
	sort.SliceStable(cands, func(i, j int) bool {
		if cands[i].private != cands[j].private {
			return cands[i].private
		}
		if cands[i].name != cands[j].name {
			return cands[i].name < cands[j].name
		}
		return bytes.Compare(cands[i].ip, cands[j].ip) < 0
	})
	chosen := cands[0].ip
	log.Printf("pickLANIPv4: using %s (iface_hint=%s)", chosen, cands[0].name)
	return chosen
}

func normalizePublicBase(s string) string {
	s = strings.TrimSpace(s)
	s = strings.TrimSuffix(s, "/")
	return s
}

func logStartupURLs(listenAddr, portStr string) {
	log.Printf("=== local_gateway (plaintext mock) ===")
	log.Printf("listen tcp4: %s", listenAddr)
	log.Printf("POST /v1/updater_endpoint will return: {\"url\": %q}", publicUpdaterBaseURL)
	log.Printf("Point AmneziaVPN gateway setting to: %s/", publicUpdaterBaseURL)
	log.Printf("Try from phone browser: %s/", publicUpdaterBaseURL)
	log.Printf("Non-loopback IPv4 URLs (same listen port %s):", portStr)
	ifaces, err := net.Interfaces()
	if err != nil {
		log.Printf("  (could not enumerate interfaces: %v)", err)
	} else {
		for _, iface := range ifaces {
			if iface.Flags&net.FlagUp == 0 || iface.Flags&net.FlagLoopback != 0 {
				continue
			}
			addrs, _ := iface.Addrs()
			for _, a := range addrs {
				ipNet, ok := a.(*net.IPNet)
				if !ok {
					continue
				}
				if ip4 := ipNet.IP.To4(); ip4 != nil && !ip4.IsLoopback() {
					log.Printf("  http://%s:%s/  (iface %s)", ip4, portStr, iface.Name)
				}
			}
		}
	}
	log.Printf("docs: tools/local_gateway/README.md tools/local_gateway/LAN_GATEWAY.md")
	log.Printf("========================================")
}

func main() {
	listenFlag := flag.String("listen", envOrDefault("LOCAL_GATEWAY_LISTEN", "0.0.0.0:8080"),
		"TCP listen address (tcp4). Env: LOCAL_GATEWAY_LISTEN")
	publicFlag := flag.String("public-base", strings.TrimSpace(os.Getenv("LOCAL_GATEWAY_PUBLIC_BASE")),
		"Base URL without trailing slash for /v1/updater_endpoint (required for iOS-on-LAN). Env: LOCAL_GATEWAY_PUBLIC_BASE")
	autoPublic := flag.Bool("auto-public", true, "If public-base empty, derive http://<first-lan-ipv4>:port")
	pairTTL := flag.Duration("pairing-ttl", 120*time.Second, "QR pairing session TTL")
	longPoll := flag.Duration("long-poll", 120*time.Second, "Long-poll max wait for POST /api/v1/generate_qr")
	rateN := flag.Int("rate-limit-excess-after", 0, "Amnezia Free: allow N requests per 24h window before rate-limit/CAPTCHA (0=tight)")
	flag.Parse()

	listenAddr := strings.TrimSpace(*listenFlag)
	if _, _, err := net.SplitHostPort(listenAddr); err != nil {
		listenAddr = net.JoinHostPort(listenAddr, "8080")
	}
	_, portStr, err := net.SplitHostPort(listenAddr)
	if err != nil {
		log.Fatalf("listen address: %v", err)
	}

	pairingSessionTTL = *pairTTL
	longPollWaitLimit = *longPoll
	rateLimitExcessAfter = *rateN

	pub := normalizePublicBase(*publicFlag)
	if pub == "" && *autoPublic {
		if ip := pickLANIPv4(); ip != nil {
			pub = fmt.Sprintf("http://%s:%s", ip.String(), portStr)
			log.Printf("auto-public: updater + docs base -> %s (override with -public-base or LOCAL_GATEWAY_PUBLIC_BASE)", pub)
		}
	}
	if pub == "" {
		pub = fmt.Sprintf("http://127.0.0.1:%s", portStr)
		log.Printf("WARN: public-base not set and auto-public found no LAN IPv4; using %s (broken for remote phones). Set -public-base or LOCAL_GATEWAY_PUBLIC_BASE.", pub)
	}
	publicUpdaterBaseURL = pub

	http.HandleFunc("/", logReq(handleRoot))
	http.HandleFunc("/VERSION", logReq(handleGetVersion))
	http.HandleFunc("/CHANGELOG", logReq(handleGetChangelog))
	http.HandleFunc("/RELEASE_DATE", logReq(handleGetReleaseDate))
	http.HandleFunc("/v1/account_info", logReq(handleAccountInfo))
	http.HandleFunc("/v1/services", logReq(handleServices))
	http.HandleFunc("/v1/config", logReq(handleConfig))
	http.HandleFunc("/v1/news", logReq(handleNews))
	http.HandleFunc("/v1/renewal_link", logReq(handleRenewalLink))
	http.HandleFunc("/v1/updater_endpoint", logReq(handleUpdaterEndpoint))
	http.HandleFunc("/v1/revoke_config", logReq(handleRevokeNoop))
	http.HandleFunc("/v1/revoke_native_config", logReq(handleRevokeNoop))
	http.HandleFunc("/api/v1/generate_qr", logReq(handleGenerateQR))
	http.HandleFunc("/api/v1/scan_qr", logReq(handleScanQR))

	logStartupURLs(listenAddr, portStr)

	ln, err := net.Listen("tcp4", listenAddr)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("listening tcp4 %s (actual %v)", listenAddr, ln.Addr())
	log.Fatal(http.Serve(ln, nil))
}
