package handlers

import (
	"crypto/rand"
	"crypto/sha256"
	"crypto/subtle"
	"encoding/base64"
	"fmt"
	"html"
	"math/big"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"vpn-backend/internal/config"
	"vpn-backend/internal/middleware"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/golang-jwt/jwt/v5"
	"golang.org/x/crypto/bcrypt"
	"gorm.io/gorm"
)

type AuthHandler struct {
	db                *gorm.DB
	cfg               *config.Config
	jwtSecret         string
	tvRateLimitMu     sync.Mutex
	tvApproveAttempts map[string][]time.Time
	tvTokenAttempts   map[string][]time.Time
}

func NewAuthHandler(db *gorm.DB, cfg *config.Config) *AuthHandler {
	return &AuthHandler{
		db:                db,
		cfg:               cfg,
		jwtSecret:         cfg.JWTSecret,
		tvApproveAttempts: make(map[string][]time.Time),
		tvTokenAttempts:   make(map[string][]time.Time),
	}
}

type registerRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required,min=8"`
}

type loginRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required"`
}

type tvStartResponse struct {
	DeviceCode              string `json:"device_code"`
	UserCode                string `json:"user_code"`
	VerificationURI         string `json:"verification_uri"`
	VerificationURIComplete string `json:"verification_uri_complete"`
	ExpiresIn               int    `json:"expires_in"`
	Interval                int    `json:"interval"`
}

type tvApproveRequest struct {
	UserCode string `json:"user_code" binding:"required"`
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required"`
}

type tvApproveAuthenticatedRequest struct {
	UserCode string `json:"user_code" binding:"required"`
}

type tvTokenRequest struct {
	DeviceCode string `json:"device_code" binding:"required"`
}

type tokenResponse struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
}

func generateCode() string {
	n, _ := rand.Int(rand.Reader, big.NewInt(900000))
	return fmt.Sprintf("%06d", n.Int64()+100000)
}

func generateTVUserCode() string {
	const alphabet = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789"
	var out strings.Builder
	for i := 0; i < 8; i++ {
		n, _ := rand.Int(rand.Reader, big.NewInt(int64(len(alphabet))))
		out.WriteByte(alphabet[n.Int64()])
	}
	return out.String()
}

func generateTVDeviceCode() string {
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return ""
	}
	return base64.RawURLEncoding.EncodeToString(buf)
}

func normalizeTVUserCode(code string) string {
	return strings.ToUpper(strings.ReplaceAll(strings.TrimSpace(code), "-", ""))
}

func (h *AuthHandler) hashTVCode(code string) string {
	sum := sha256.Sum256([]byte(h.jwtSecret + ":" + code))
	return base64.RawURLEncoding.EncodeToString(sum[:])
}

func requestBaseURL(c *gin.Context) string {
	proto := c.GetHeader("X-Forwarded-Proto")
	if proto == "" {
		if c.Request.TLS != nil {
			proto = "https"
		} else {
			proto = "http"
		}
	}
	host := c.GetHeader("X-Forwarded-Host")
	if host == "" {
		host = c.Request.Host
	}
	return proto + "://" + host
}

func (h *AuthHandler) expireStaleTVLogins() {
	now := time.Now()
	h.db.Model(&models.TVLogin{}).
		Where("status IN ? AND expires_at <= ?", []models.TVLoginStatus{models.TVLoginPending, models.TVLoginApproved}, now).
		Update("status", models.TVLoginExpired)
}

func (h *AuthHandler) allowTVAttempt(bucket map[string][]time.Time, key string, limit int, window time.Duration) bool {
	now := time.Now()
	cutoff := now.Add(-window)

	h.tvRateLimitMu.Lock()
	defer h.tvRateLimitMu.Unlock()

	attempts := bucket[key]
	kept := attempts[:0]
	for _, at := range attempts {
		if at.After(cutoff) {
			kept = append(kept, at)
		}
	}
	if len(kept) >= limit {
		bucket[key] = kept
		return false
	}
	bucket[key] = append(kept, now)
	return true
}

// POST /api/v1/auth/tv/start
func (h *AuthHandler) TVStart(c *gin.Context) {
	h.expireStaleTVLogins()

	deviceCode := generateTVDeviceCode()
	if deviceCode == "" {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to generate device code"})
		return
	}
	userCode := generateTVUserCode()
	now := time.Now()
	login := models.TVLogin{
		DeviceCodeHash: h.hashTVCode(deviceCode),
		UserCodeHash:   h.hashTVCode(userCode),
		Status:         models.TVLoginPending,
		ExpiresAt:      now.Add(10 * time.Minute),
	}
	if err := h.db.Create(&login).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to create tv login"})
		return
	}

	// We hand the device the API-prefixed URL because reverse proxies in
	// production are typically configured to forward only /api/v1/* to
	// the backend. The same handler is also mounted at /tv for direct
	// access without a proxy.
	baseURL := requestBaseURL(c)
	verificationURI := baseURL + "/api/v1/tv"
	c.JSON(http.StatusOK, tvStartResponse{
		DeviceCode:              deviceCode,
		UserCode:                userCode[:4] + "-" + userCode[4:],
		VerificationURI:         verificationURI,
		VerificationURIComplete: verificationURI + "?code=" + userCode,
		ExpiresIn:               600,
		Interval:                8,
	})
}

// POST /api/v1/auth/tv/approve
func (h *AuthHandler) TVApprove(c *gin.Context) {
	h.expireStaleTVLogins()

	var req tvApproveRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	userCode := normalizeTVUserCode(req.UserCode)
	if !h.allowTVAttempt(h.tvApproveAttempts, "approve-ip:"+c.ClientIP(), 60, 10*time.Minute) ||
		!h.allowTVAttempt(h.tvApproveAttempts, "approve-code:"+c.ClientIP()+":"+userCode, 6, 10*time.Minute) {
		c.JSON(http.StatusTooManyRequests, gin.H{"error": "too_many_attempts"})
		return
	}

	var login models.TVLogin
	if err := h.db.Where("user_code_hash = ? AND status = ? AND expires_at > ?",
		h.hashTVCode(userCode), models.TVLoginPending, time.Now()).First(&login).Error; err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid or expired code"})
		return
	}

	var user models.User
	if err := h.db.Where("email = ?", req.Email).First(&user).Error; err != nil {
		if isRecordNotFoundError(err) {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid email or password"})
			return
		}
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user"})
		return
	}

	if err := bcrypt.CompareHashAndPassword([]byte(user.PasswordHash), []byte(req.Password)); err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid email or password"})
		return
	}

	now := time.Now()
	if err := h.db.Model(&login).Updates(map[string]interface{}{
		"user_id":     user.ID,
		"status":      models.TVLoginApproved,
		"approved_at": &now,
	}).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to approve tv login"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"status": "approved"})
}

// POST /api/v1/me/tv/approve
//
// Same effect as TVApprove, but authenticates the user via the existing
// JWT instead of asking for email+password again. Designed for the
// "I have a TV" flow inside the FBLink mobile/desktop client: the user
// is already signed in there, they just type the code shown on the TV
// and tap Confirm.
func (h *AuthHandler) TVApproveAuthenticated(c *gin.Context) {
	h.expireStaleTVLogins()

	userID := c.GetUint("user_id")
	if userID == 0 {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "unauthorized"})
		return
	}

	var req tvApproveAuthenticatedRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	userCode := normalizeTVUserCode(req.UserCode)
	if !h.allowTVAttempt(h.tvApproveAttempts, "approve-user:"+strconv.FormatUint(uint64(userID), 10), 60, 10*time.Minute) ||
		!h.allowTVAttempt(h.tvApproveAttempts, "approve-user-code:"+strconv.FormatUint(uint64(userID), 10)+":"+userCode, 8, 10*time.Minute) {
		c.JSON(http.StatusTooManyRequests, gin.H{"error": "too_many_attempts"})
		return
	}

	var login models.TVLogin
	if err := h.db.Where("user_code_hash = ? AND status = ? AND expires_at > ?",
		h.hashTVCode(userCode), models.TVLoginPending, time.Now()).First(&login).Error; err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid or expired code"})
		return
	}

	var user models.User
	if err := h.db.First(&user, userID).Error; err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "user not found"})
		return
	}

	now := time.Now()
	if err := h.db.Model(&login).Updates(map[string]interface{}{
		"user_id":     user.ID,
		"status":      models.TVLoginApproved,
		"approved_at": &now,
	}).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to approve tv login"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"status": "approved"})
}

// POST /api/v1/auth/tv/token
func (h *AuthHandler) TVToken(c *gin.Context) {
	h.expireStaleTVLogins()

	var req tvTokenRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	deviceHash := h.hashTVCode(req.DeviceCode)
	if !h.allowTVAttempt(h.tvTokenAttempts, "token-ip:"+c.ClientIP(), 180, 10*time.Minute) {
		c.JSON(http.StatusTooManyRequests, gin.H{"error": "slow_down", "interval": 8})
		return
	}

	var login models.TVLogin
	if err := h.db.Where("device_code_hash = ?", deviceHash).First(&login).Error; err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid device code"})
		return
	}
	if !h.allowTVAttempt(h.tvTokenAttempts, "token-device:"+deviceHash, 120, 10*time.Minute) {
		c.JSON(http.StatusTooManyRequests, gin.H{"error": "slow_down", "interval": 8})
		return
	}
	if login.Status == models.TVLoginExpired || time.Now().After(login.ExpiresAt) {
		h.db.Model(&login).Update("status", models.TVLoginExpired)
		c.JSON(http.StatusBadRequest, gin.H{"error": "expired_token"})
		return
	}
	if login.Status == models.TVLoginConsumed {
		c.JSON(http.StatusBadRequest, gin.H{"error": "already_used"})
		return
	}
	if login.Status == models.TVLoginPending {
		now := time.Now()
		if login.LastPolledAt != nil && now.Sub(*login.LastPolledAt) < 3*time.Second {
			c.JSON(http.StatusTooManyRequests, gin.H{"error": "slow_down", "interval": 8})
			return
		}
		h.db.Model(&login).Update("last_polled_at", &now)
		c.JSON(http.StatusAccepted, gin.H{"status": "pending", "error": "authorization_pending"})
		return
	}
	if login.UserID == nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "approved login is missing user"})
		return
	}

	var user models.User
	if err := h.db.First(&user, *login.UserID).Error; err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "user not found"})
		return
	}

	now := time.Now()
	result := h.db.Model(&models.TVLogin{}).Where("id = ? AND status = ?", login.ID, models.TVLoginApproved).Updates(map[string]interface{}{
		"status":      models.TVLoginConsumed,
		"consumed_at": &now,
	})
	if result.Error != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to consume tv login"})
		return
	}
	if result.RowsAffected == 0 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "already_used"})
		return
	}

	tokens, err := h.generateTokens(&user)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to generate tokens"})
		return
	}
	c.JSON(http.StatusOK, tokens)
}

// GET /tv
func (h *AuthHandler) TVApprovePage(c *gin.Context) {
	code := html.EscapeString(c.Query("code"))
	c.Header("Content-Type", "text/html; charset=utf-8")
	c.String(http.StatusOK, `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>FBLink TV Login</title>
  <style>
    body{margin:0;background:#09090b;color:#f4f4f5;font-family:system-ui,-apple-system,Segoe UI,sans-serif;display:grid;min-height:100vh;place-items:center}
    form{width:min(420px,calc(100vw - 32px));background:#111114;border:1px solid #2f2f35;border-radius:18px;padding:24px;box-shadow:0 18px 60px rgba(0,0,0,.35)}
    h1{margin:0 0 8px;font-size:26px} p{color:#a1a1aa;margin:0 0 18px;line-height:1.45}
    label{display:block;margin-top:14px;color:#d4d4d8;font-size:14px} input{box-sizing:border-box;width:100%;margin-top:6px;padding:13px 14px;border-radius:12px;border:1px solid #3f3f46;background:#09090b;color:#fff;font-size:16px}
    button{width:100%;margin-top:18px;padding:14px;border:0;border-radius:12px;background:#eab308;color:#111;font-weight:700;font-size:16px}
    #msg{margin-top:14px;min-height:22px;color:#fbbf24}
  </style>
</head>
<body>
  <form id="form">
    <h1>FBLink VPN TV</h1>
    <p>Enter the code from your TV and your account credentials to sign in on Android TV.</p>
    <label>Code</label><input id="code" autocomplete="one-time-code" value="`+code+`" required>
    <label>Email</label><input id="email" type="email" autocomplete="email" required>
    <label>Password</label><input id="password" type="password" autocomplete="current-password" required>
    <button>Confirm sign in</button>
    <div id="msg"></div>
  </form>
  <script>
    const form = document.getElementById('form'), msg = document.getElementById('msg');
    form.addEventListener('submit', async e => {
      e.preventDefault(); msg.textContent = 'Checking...';
      const res = await fetch('/api/v1/auth/tv/approve', {
        method: 'POST', headers: {'Content-Type':'application/json'},
        body: JSON.stringify({
          user_code: document.getElementById('code').value,
          email: document.getElementById('email').value,
          password: document.getElementById('password').value
        })
      });
      const data = await res.json().catch(() => ({}));
      msg.textContent = res.ok ? 'Done. You can return to the TV.' : (data.error || 'Confirmation failed');
    });
  </script>
</body>
</html>`)
}

// POST /api/v1/auth/register
func (h *AuthHandler) Register(c *gin.Context) {
	var req registerRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Проверить, что email не занят
	var existing models.User
	if err := h.db.Where("email = ?", req.Email).First(&existing).Error; err == nil {
		c.JSON(http.StatusConflict, gin.H{"error": "email already registered"})
		return
	} else if !isRecordNotFoundError(err) {
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to check email availability"})
		return
	}

	// Проверить cooldown: не отправлять код чаще раза в 60 секунд (на бекенде даем 55 сек для компенсации таймеров клиента)
	var recent models.VerificationCode
	cooldown := time.Now().Add(-55 * time.Second)
	if h.db.Where("email = ? AND purpose = ? AND created_at > ? AND used = false", req.Email, "verify", cooldown).
		First(&recent).Error == nil {
		c.JSON(http.StatusTooManyRequests, gin.H{"error": "code already sent, wait 60 seconds before retrying"})
		return
	}

	// Инвалидировать старые коды
	h.db.Where("email = ? AND purpose = ? AND used = false", req.Email, "verify").
		Update("used", true)

	hash, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "internal error"})
		return
	}

	code := generateCode()
	vc := models.VerificationCode{
		Email:     req.Email,
		Code:      code + "|" + string(hash), // code|bcrypt_hash
		Purpose:   "verify",
		ExpiresAt: time.Now().Add(10 * time.Minute),
	}
	h.db.Create(&vc)

	sendVerificationEmailAsync(h.cfg, req.Email, code, "verify")
	c.JSON(http.StatusOK, gin.H{"message": "verification code sent to email"})
}

type verifyRequest struct {
	Email string `json:"email" binding:"required,email"`
	Code  string `json:"code" binding:"required"`
}

// POST /api/v1/auth/verify
func (h *AuthHandler) VerifyEmail(c *gin.Context) {
	var req verifyRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var vc models.VerificationCode
	if err := h.db.Where("email = ? AND purpose = ? AND used = false AND expires_at > ?",
		req.Email, "verify", time.Now()).First(&vc).Error; err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid or expired code"})
		return
	}

	// Парсим code|hash
	parts := strings.SplitN(vc.Code, "|", 2)
	if len(parts) != 2 || subtle.ConstantTimeCompare([]byte(parts[0]), []byte(req.Code)) != 1 {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid or expired code"})
		return
	}

	// Помечаем код использованным
	h.db.Model(&vc).Update("used", true)

	// Создаём юзера
	user := models.User{
		Email:        req.Email,
		PasswordHash: parts[1],
		Role:         models.RoleUser,
	}
	if err := h.db.Create(&user).Error; err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": "email already registered"})
		return
	}

	sub := models.Subscription{
		UserID:    user.ID,
		Plan:      models.PlanFree,
		Status:    models.SubActive,
		ExpiresAt: time.Now().AddDate(1, 0, 0),
	}
	h.db.Create(&sub)

	tokens, err := h.generateTokens(&user)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "internal error"})
		return
	}

	c.JSON(http.StatusCreated, tokens)
}

type forgotPasswordRequest struct {
	Email string `json:"email" binding:"required,email"`
}

// POST /api/v1/auth/forgot-password
func (h *AuthHandler) ForgotPassword(c *gin.Context) {
	var req forgotPasswordRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var user models.User
	if err := h.db.Where("email = ?", req.Email).First(&user).Error; err != nil {
		if isRecordNotFoundError(err) {
			c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
			return
		}
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to process password reset request"})
		return
	}

	var recent models.VerificationCode
	cooldown := time.Now().Add(-55 * time.Second)
	if h.db.Where("email = ? AND purpose = ? AND created_at > ? AND used = false", req.Email, "reset", cooldown).
		First(&recent).Error == nil {
		c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
		return
	}

	h.db.Where("email = ? AND purpose = ? AND used = false", req.Email, "reset").
		Update("used", true)

	code := generateCode()
	vc := models.VerificationCode{
		Email:     req.Email,
		Code:      code,
		Purpose:   "reset",
		ExpiresAt: time.Now().Add(10 * time.Minute),
	}
	h.db.Create(&vc)

	sendVerificationEmailAsync(h.cfg, req.Email, code, "reset")
	c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
}

type resetPasswordRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Code     string `json:"code" binding:"required"`
	Password string `json:"new_password" binding:"required,min=8"`
}

// POST /api/v1/auth/reset-password
func (h *AuthHandler) ResetPassword(c *gin.Context) {
	var req resetPasswordRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var vc models.VerificationCode
	if err := h.db.Where("email = ? AND purpose = ? AND code = ? AND used = false AND expires_at > ?",
		req.Email, "reset", req.Code, time.Now()).First(&vc).Error; err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid or expired code"})
		return
	}

	h.db.Model(&vc).Update("used", true)

	hash, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "internal error"})
		return
	}

	result := h.db.Model(&models.User{}).Where("email = ?", req.Email).
		Update("password_hash", string(hash))
	if result.Error != nil || result.RowsAffected == 0 {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update password"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "password updated successfully"})
}

// POST /api/v1/auth/login
func (h *AuthHandler) Login(c *gin.Context) {
	var req loginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var user models.User
	if err := h.db.Where("email = ?", req.Email).First(&user).Error; err != nil {
		if isRecordNotFoundError(err) {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid email or password"})
			return
		}
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user"})
		return
	}

	if err := bcrypt.CompareHashAndPassword([]byte(user.PasswordHash), []byte(req.Password)); err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid email or password"})
		return
	}

	tokens, err := h.generateTokens(&user)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to generate tokens"})
		return
	}

	c.JSON(http.StatusOK, tokens)
}

// POST /api/v1/auth/refresh
func (h *AuthHandler) Refresh(c *gin.Context) {
	var body struct {
		RefreshToken string `json:"refresh_token" binding:"required"`
	}
	if err := c.ShouldBindJSON(&body); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	token, err := jwt.ParseWithClaims(body.RefreshToken, &middleware.Claims{}, func(t *jwt.Token) (interface{}, error) {
		return []byte(h.jwtSecret), nil
	})
	if err != nil || !token.Valid {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid refresh token"})
		return
	}

	claims := token.Claims.(*middleware.Claims)
	var user models.User
	if err := h.db.First(&user, claims.UserID).Error; err != nil {
		if isRecordNotFoundError(err) {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "user not found"})
			return
		}
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load user"})
		return
	}

	tokens, err := h.generateTokens(&user)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to generate tokens"})
		return
	}

	c.JSON(http.StatusOK, tokens)
}

func (h *AuthHandler) generateTokens(user *models.User) (*tokenResponse, error) {
	accessClaims := middleware.Claims{
		UserID: user.ID,
		Role:   user.Role,
		RegisteredClaims: jwt.RegisteredClaims{
			// Access token: короткий TTL — 1 час (уменьшает ущерб при утечке)
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(1 * time.Hour)),
		},
	}
	accessToken := jwt.NewWithClaims(jwt.SigningMethodHS256, accessClaims)
	accessStr, err := accessToken.SignedString([]byte(h.jwtSecret))
	if err != nil {
		return nil, err
	}

	refreshClaims := middleware.Claims{
		UserID: user.ID,
		Role:   user.Role,
		RegisteredClaims: jwt.RegisteredClaims{
			// Refresh token: долгий TTL — 30 дней
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(30 * 24 * time.Hour)),
		},
	}
	refreshToken := jwt.NewWithClaims(jwt.SigningMethodHS256, refreshClaims)
	refreshStr, err := refreshToken.SignedString([]byte(h.jwtSecret))
	if err != nil {
		return nil, err
	}

	return &tokenResponse{
		AccessToken:  accessStr,
		RefreshToken: refreshStr,
	}, nil
}
