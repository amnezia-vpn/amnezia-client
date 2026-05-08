package handlers

import (
	"bytes"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"testing"
	"time"

	"vpn-backend/internal/config"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/glebarez/sqlite"
	"golang.org/x/crypto/bcrypt"
	"gorm.io/gorm"
)

func openTVLoginDB(t *testing.T) *gorm.DB {
	t.Helper()

	dbPath := filepath.Join(t.TempDir(), "tv-login.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}
	if err := db.AutoMigrate(&models.User{}, &models.Subscription{}, &models.TVLogin{}).Error; err != nil {
		t.Fatalf("migrate: %v", err)
	}

	sqlDB, err := db.DB()
	if err != nil {
		t.Fatalf("db handle: %v", err)
	}
	t.Cleanup(func() { _ = sqlDB.Close() })
	return db
}

func newTVLoginRouter(db *gorm.DB) *gin.Engine {
	gin.SetMode(gin.TestMode)
	cfg := &config.Config{JWTSecret: "test-secret"}
	h := NewAuthHandler(db, cfg)
	r := gin.New()
	r.POST("/api/v1/auth/tv/start", h.TVStart)
	r.POST("/api/v1/auth/tv/approve", h.TVApprove)
	r.POST("/api/v1/auth/tv/token", h.TVToken)
	return r
}

func postJSON(t *testing.T, r http.Handler, path string, payload any) *httptest.ResponseRecorder {
	t.Helper()
	body, err := json.Marshal(payload)
	if err != nil {
		t.Fatalf("marshal: %v", err)
	}
	req := httptest.NewRequest(http.MethodPost, path, bytes.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()
	r.ServeHTTP(w, req)
	return w
}

func seedTVLoginUser(t *testing.T, db *gorm.DB) {
	t.Helper()
	hash, err := bcrypt.GenerateFromPassword([]byte("password123"), bcrypt.DefaultCost)
	if err != nil {
		t.Fatalf("hash password: %v", err)
	}
	if err := db.Create(&models.User{
		Email:        "tv@example.com",
		PasswordHash: string(hash),
		Role:         models.RoleUser,
	}).Error; err != nil {
		t.Fatalf("create user: %v", err)
	}
}

func TestTVLoginStartStoresOnlyHashes(t *testing.T) {
	db := openTVLoginDB(t)
	r := newTVLoginRouter(db)

	w := postJSON(t, r, "/api/v1/auth/tv/start", gin.H{})
	if w.Code != http.StatusOK {
		t.Fatalf("status = %d, body = %s", w.Code, w.Body.String())
	}

	var resp struct {
		DeviceCode string `json:"device_code"`
		UserCode   string `json:"user_code"`
		ExpiresIn  int    `json:"expires_in"`
	}
	if err := json.Unmarshal(w.Body.Bytes(), &resp); err != nil {
		t.Fatalf("decode response: %v", err)
	}
	if resp.DeviceCode == "" || resp.UserCode == "" || resp.ExpiresIn != 600 {
		t.Fatalf("unexpected start response: %+v", resp)
	}

	var login models.TVLogin
	if err := db.First(&login).Error; err != nil {
		t.Fatalf("load tv login: %v", err)
	}
	if login.DeviceCodeHash == resp.DeviceCode || login.UserCodeHash == resp.UserCode {
		t.Fatalf("stored raw tv codes instead of hashes")
	}
	if login.Status != models.TVLoginPending || time.Until(login.ExpiresAt) <= 0 {
		t.Fatalf("unexpected login state: %+v", login)
	}
}

func TestTVLoginApproveAndPollTokens(t *testing.T) {
	db := openTVLoginDB(t)
	seedTVLoginUser(t, db)
	r := newTVLoginRouter(db)

	start := postJSON(t, r, "/api/v1/auth/tv/start", gin.H{})
	var startResp struct {
		DeviceCode string `json:"device_code"`
		UserCode   string `json:"user_code"`
	}
	_ = json.Unmarshal(start.Body.Bytes(), &startResp)

	pending := postJSON(t, r, "/api/v1/auth/tv/token", gin.H{"device_code": startResp.DeviceCode})
	if pending.Code != http.StatusAccepted {
		t.Fatalf("pending status = %d, body = %s", pending.Code, pending.Body.String())
	}

	badApprove := postJSON(t, r, "/api/v1/auth/tv/approve", gin.H{
		"user_code": startResp.UserCode,
		"email":     "tv@example.com",
		"password":  "bad-password",
	})
	if badApprove.Code != http.StatusUnauthorized {
		t.Fatalf("bad approve status = %d", badApprove.Code)
	}

	approve := postJSON(t, r, "/api/v1/auth/tv/approve", gin.H{
		"user_code": startResp.UserCode,
		"email":     "tv@example.com",
		"password":  "password123",
	})
	if approve.Code != http.StatusOK {
		t.Fatalf("approve status = %d, body = %s", approve.Code, approve.Body.String())
	}

	token := postJSON(t, r, "/api/v1/auth/tv/token", gin.H{"device_code": startResp.DeviceCode})
	if token.Code != http.StatusOK {
		t.Fatalf("token status = %d, body = %s", token.Code, token.Body.String())
	}
	var tokenResp map[string]string
	_ = json.Unmarshal(token.Body.Bytes(), &tokenResp)
	if tokenResp["access_token"] == "" || tokenResp["refresh_token"] == "" {
		t.Fatalf("missing tokens: %+v", tokenResp)
	}

	again := postJSON(t, r, "/api/v1/auth/tv/token", gin.H{"device_code": startResp.DeviceCode})
	if again.Code != http.StatusBadRequest {
		t.Fatalf("consumed token status = %d", again.Code)
	}
}

func TestTVLoginExpiredCodeRejected(t *testing.T) {
	db := openTVLoginDB(t)
	r := newTVLoginRouter(db)

	start := postJSON(t, r, "/api/v1/auth/tv/start", gin.H{})
	var startResp struct {
		DeviceCode string `json:"device_code"`
	}
	_ = json.Unmarshal(start.Body.Bytes(), &startResp)

	if err := db.Model(&models.TVLogin{}).Update("expires_at", time.Now().Add(-time.Minute)).Error; err != nil {
		t.Fatalf("expire login: %v", err)
	}

	token := postJSON(t, r, "/api/v1/auth/tv/token", gin.H{"device_code": startResp.DeviceCode})
	if token.Code != http.StatusBadRequest {
		t.Fatalf("expired token status = %d, body = %s", token.Code, token.Body.String())
	}
}
