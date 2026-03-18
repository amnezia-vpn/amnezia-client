package handlers

import (
	"crypto/rand"
	"crypto/subtle"
	"fmt"
	"math/big"
	"net/http"
	"strings"
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
	db        *gorm.DB
	cfg       *config.Config
	jwtSecret string
}

func NewAuthHandler(db *gorm.DB, cfg *config.Config) *AuthHandler {
	return &AuthHandler{db: db, cfg: cfg, jwtSecret: cfg.JWTSecret}
}

type registerRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required,min=8"`
}

type loginRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Password string `json:"password" binding:"required"`
}

type tokenResponse struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
}

func generateCode() string {
	n, _ := rand.Int(rand.Reader, big.NewInt(900000))
	return fmt.Sprintf("%06d", n.Int64()+100000)
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
	if h.db.Where("email = ?", req.Email).First(&existing).Error == nil {
		c.JSON(http.StatusConflict, gin.H{"error": "email already registered"})
		return
	}

	// Проверить cooldown: не отправлять код чаще раза в 60 секунд
	var recent models.VerificationCode
	cooldown := time.Now().Add(-60 * time.Second)
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
	if h.db.Where("email = ?", req.Email).First(&user).Error != nil {
		c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
		return
	}

	var recent models.VerificationCode
	cooldown := time.Now().Add(-60 * time.Second)
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
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid email or password"})
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
		c.JSON(http.StatusUnauthorized, gin.H{"error": "user not found"})
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
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(15 * time.Minute)),
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
