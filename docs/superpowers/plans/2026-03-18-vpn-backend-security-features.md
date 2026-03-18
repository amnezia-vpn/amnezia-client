# VPN Backend — Security & Email Features Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Добавить безопасность (CORS, rate limiting, JWT валидация, webhook IP whitelist, hard delete) и email-фичи (верификация при регистрации, сброс пароля) в Go бэкенд.

**Architecture:** Каждый новый компонент — отдельный файл с одной ответственностью. Изменения в `router.go` — последний шаг (wiring). `auth.go` получает 4 новых endpoint'а. Регистрация теперь двухшаговая: POST /register → отправляет код → POST /verify → создаёт юзера.

**Tech Stack:** Go 1.21+, Gin, GORM (SQLite), net/smtp, sync.Mutex (rate limiter)

---

## Карта файлов

| Файл | Действие | Ответственность |
|------|----------|-----------------|
| `vpn-backend/internal/config/config.go` | Изменить | Добавить `AllowedOrigins`, валидация JWT secret |
| `vpn-backend/internal/models/models.go` | Изменить | Добавить модель `VerificationCode` |
| `vpn-backend/internal/database/database.go` | Изменить | Добавить `VerificationCode` в AutoMigrate |
| `vpn-backend/internal/middleware/ratelimit.go` | Создать | In-memory rate limiter (map+mutex) |
| `vpn-backend/internal/handlers/email.go` | Создать | SMTP email через STARTTLS с таймаутом |
| `vpn-backend/internal/handlers/auth.go` | Изменить | Register (отправка кода), +Verify, +ForgotPassword, +ResetPassword |
| `vpn-backend/internal/handlers/admin.go` | Изменить | Hard delete через `Unscoped()` |
| `vpn-backend/internal/router/router.go` | Изменить | CORS configurable, rate limiting, webhook IP, новые routes |
| `vpn-backend/cmd/server/main.go` | Изменить | `safeGo` panic recovery для горутин |

---

## Task 1: Config — JWT валидация и AllowedOrigins

**Файл:** `vpn-backend/internal/config/config.go`

- [ ] **Шаг 1: Добавить `AllowedOrigins` в структуру Config и убрать дефолтный JWT secret**

Заменить весь файл:

```go
package config

import (
	"log"
	"os"
	"strconv"

	"github.com/joho/godotenv"
)

type Config struct {
	Port           string
	DBPath         string
	JWTSecret      string
	AllowedOrigins string // CORS: "*" или "https://example.com,https://app.example.com"
	YooKassaShopID string
	YooKassaKey    string

	// SMTP для email
	SMTPHost            string
	SMTPPort            int
	SMTPUser            string
	SMTPPassword        string
	SMTPFrom            string
	BackupIntervalHours int
}

func Load() *Config {
	if err := godotenv.Load(); err != nil {
		log.Println("No .env file found, using environment variables")
	}

	secret := getEnv("JWT_SECRET", "")
	if secret == "" || secret == "change-me-in-production" {
		log.Fatal("[FATAL] JWT_SECRET не задан или использует дефолтное значение. Укажите безопасный секрет в .env")
	}

	smtpPort, _ := strconv.Atoi(getEnv("SMTP_PORT", "587"))
	backupInterval, _ := strconv.Atoi(getEnv("BACKUP_INTERVAL_HOURS", "24"))
	if backupInterval <= 0 {
		backupInterval = 24
	}

	return &Config{
		Port:           getEnv("PORT", "8081"),
		DBPath:         getEnv("DB_PATH", "data/vpn.db"),
		JWTSecret:      secret,
		AllowedOrigins: getEnv("ALLOWED_ORIGINS", "*"),
		YooKassaShopID: getEnv("YOOKASSA_SHOP_ID", ""),
		YooKassaKey:    getEnv("YOOKASSA_SECRET_KEY", ""),

		SMTPHost:            getEnv("SMTP_HOST", ""),
		SMTPPort:            smtpPort,
		SMTPUser:            getEnv("SMTP_USER", ""),
		SMTPPassword:        getEnv("SMTP_PASSWORD", ""),
		SMTPFrom:            getEnv("SMTP_FROM", ""),
		BackupIntervalHours: backupInterval,
	}
}

func getEnv(key, defaultValue string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultValue
}
```

- [ ] **Шаг 2: Убедиться что `.env` содержит реальный JWT_SECRET**

В `vpn-backend/.env` должно быть:
```
JWT_SECRET=<реальный секрет, не change-me>
```

- [ ] **Шаг 3: Commit**

```bash
git add vpn-backend/internal/config/config.go
git commit -m "security: require non-default JWT_SECRET, add AllowedOrigins config"
```

---

## Task 2: Rate limiter middleware

**Файл:** `vpn-backend/internal/middleware/ratelimit.go` (создать)

- [ ] **Шаг 1: Создать файл rate limiter'а**

```go
package middleware

import (
	"net/http"
	"sync"
	"time"

	"github.com/gin-gonic/gin"
)

type visitor struct {
	count   int
	resetAt time.Time
}

type RateLimiter struct {
	mu       sync.Mutex
	visitors map[string]*visitor
	limit    int
	window   time.Duration
}

func NewRateLimiter(limit int, window time.Duration) *RateLimiter {
	rl := &RateLimiter{
		visitors: make(map[string]*visitor),
		limit:    limit,
		window:   window,
	}
	go rl.cleanup()
	return rl
}

func (rl *RateLimiter) cleanup() {
	for {
		time.Sleep(rl.window)
		rl.mu.Lock()
		now := time.Now()
		for ip, v := range rl.visitors {
			if now.After(v.resetAt) {
				delete(rl.visitors, ip)
			}
		}
		rl.mu.Unlock()
	}
}

func (rl *RateLimiter) Allow(ip string) bool {
	rl.mu.Lock()
	defer rl.mu.Unlock()

	now := time.Now()
	v, exists := rl.visitors[ip]
	if !exists || now.After(v.resetAt) {
		rl.visitors[ip] = &visitor{count: 1, resetAt: now.Add(rl.window)}
		return true
	}

	v.count++
	return v.count <= rl.limit
}

func RateLimit(rl *RateLimiter) gin.HandlerFunc {
	return func(c *gin.Context) {
		ip := c.ClientIP()
		if !rl.Allow(ip) {
			c.AbortWithStatusJSON(http.StatusTooManyRequests, gin.H{
				"error": "too many requests, try again later",
			})
			return
		}
		c.Next()
	}
}
```

- [ ] **Шаг 2: Commit**

```bash
git add vpn-backend/internal/middleware/ratelimit.go
git commit -m "feat: add in-memory rate limiter middleware"
```

---

## Task 3: Email sender utility

**Файл:** `vpn-backend/internal/handlers/email.go` (создать)

- [ ] **Шаг 1: Создать email handler**

```go
package handlers

import (
	"crypto/tls"
	"fmt"
	"log"
	"net"
	"net/smtp"
	"strings"
	"time"
	"vpn-backend/internal/config"
)

const smtpTimeout = 15 * time.Second

// sendVerificationEmailAsync отправляет email в горутине (не блокирует UI)
func sendVerificationEmailAsync(cfg *config.Config, toEmail, code, purpose string) {
	go func() {
		if err := sendVerificationEmail(cfg, toEmail, code, purpose); err != nil {
			log.Printf("[ERROR] Failed to send %s email to %s: %v", purpose, toEmail, err)
		} else {
			log.Printf("[EMAIL] %s code sent to %s", purpose, toEmail)
		}
	}()
}

func sendVerificationEmail(cfg *config.Config, toEmail, code, purpose string) error {
	if cfg.SMTPHost == "" {
		return fmt.Errorf("SMTP not configured")
	}

	from := cfg.SMTPUser
	displayFrom := cfg.SMTPFrom
	if displayFrom == "" {
		displayFrom = from
	}

	subject := "FBLink VPN — код подтверждения"
	body := fmt.Sprintf("Ваш код подтверждения: %s\n\nКод действителен 10 минут.", code)
	if purpose == "reset" {
		subject = "FBLink VPN — восстановление пароля"
		body = fmt.Sprintf("Ваш код для сброса пароля: %s\n\nКод действителен 10 минут.\n\nЕсли вы не запрашивали сброс пароля, проигнорируйте это письмо.", code)
	}

	msg := strings.Join([]string{
		"From: " + displayFrom,
		"To: " + toEmail,
		"Subject: " + subject,
		"MIME-Version: 1.0",
		"Content-Type: text/plain; charset=UTF-8",
		"",
		body,
	}, "\r\n")

	addr := net.JoinHostPort(cfg.SMTPHost, fmt.Sprintf("%d", cfg.SMTPPort))

	conn, err := net.DialTimeout("tcp", addr, smtpTimeout)
	if err != nil {
		return fmt.Errorf("SMTP dial: %w", err)
	}

	// Устанавливаем дедлайн ДО создания SMTP клиента, чтобы он применялся ко всем операциям
	if tc, ok := conn.(*net.TCPConn); ok {
		tc.SetDeadline(time.Now().Add(30 * time.Second))
	}

	client, err := smtp.NewClient(conn, cfg.SMTPHost)
	if err != nil {
		conn.Close()
		return fmt.Errorf("SMTP client: %w", err)
	}
	defer client.Close()

	tlsConfig := &tls.Config{ServerName: cfg.SMTPHost}
	if err := client.StartTLS(tlsConfig); err != nil {
		return fmt.Errorf("STARTTLS: %w", err)
	}

	auth := smtp.PlainAuth("", cfg.SMTPUser, cfg.SMTPPassword, cfg.SMTPHost)
	if err := client.Auth(auth); err != nil {
		return fmt.Errorf("SMTP auth: %w", err)
	}

	if err := client.Mail(from); err != nil {
		return fmt.Errorf("SMTP MAIL FROM: %w", err)
	}
	if err := client.Rcpt(toEmail); err != nil {
		return fmt.Errorf("SMTP RCPT TO: %w", err)
	}
	w, err := client.Data()
	if err != nil {
		return fmt.Errorf("SMTP DATA: %w", err)
	}
	if _, err := w.Write([]byte(msg)); err != nil {
		return fmt.Errorf("SMTP write: %w", err)
	}
	if err := w.Close(); err != nil {
		return fmt.Errorf("SMTP close data: %w", err)
	}

	return client.Quit()
}
```

- [ ] **Шаг 2: Commit**

```bash
git add vpn-backend/internal/handlers/email.go
git commit -m "feat: add SMTP email sender with STARTTLS and timeout"
```

---

## Task 4: VerificationCode модель + миграция

**Файлы:** `models/models.go`, `database/database.go`

- [ ] **Шаг 1: Добавить модель `VerificationCode` в конец `models.go`**

```go
type VerificationCode struct {
	gorm.Model
	Email     string    `gorm:"not null;index"`
	Code      string    `gorm:"not null"`
	Purpose   string    `gorm:"not null"` // "verify" | "reset"
	ExpiresAt time.Time `gorm:"not null"`
	Used      bool      `gorm:"default:false"`
}
```

- [ ] **Шаг 2: Добавить в `AutoMigrate` в `database.go`**

В списке моделей добавить `&models.VerificationCode{}`:

```go
err := db.AutoMigrate(
    &models.User{},
    &models.Subscription{},
    &models.VPNServer{},
    &models.VPNKey{},
    &models.Payment{},
    &models.VerificationCode{},
)
```

- [ ] **Шаг 3: Commit**

```bash
git add vpn-backend/internal/models/models.go vpn-backend/internal/database/database.go
git commit -m "feat: add VerificationCode model for email verification and password reset"
```

---

## Task 5: Auth — двухшаговая регистрация с email верификацией

**Файл:** `vpn-backend/internal/handlers/auth.go`

Изменить `NewAuthHandler` и `Register`, добавить `VerifyEmail`:

- [ ] **Шаг 1: Обновить `AuthHandler` — добавить `cfg` поле**

```go
type AuthHandler struct {
	db        *gorm.DB
	cfg       *config.Config
	jwtSecret string
}

func NewAuthHandler(db *gorm.DB, cfg *config.Config) *AuthHandler {
	return &AuthHandler{db: db, cfg: cfg, jwtSecret: cfg.JWTSecret}
}
```

> Добавить импорт `"vpn-backend/internal/config"`.

- [ ] **Шаг 2: Изменить `Register` — не создавать юзера, только отправить код**

```go
// POST /api/v1/auth/register
func (h *AuthHandler) Register(c *gin.Context) {
	var req registerRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid request"})
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

	// Хешировать пароль заранее — сохраним в код (временно в поле Code хранить hash)
	// Нет — лучше создадим запись в VerificationCode с email+пароль-хеш отдельно
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
```

- [ ] **Шаг 3: Добавить хелпер `generateCode` (6 цифр)**

```go
import (
    "crypto/rand"
    "math/big"
    "fmt"
)

func generateCode() string {
	n, _ := rand.Int(rand.Reader, big.NewInt(900000))
	return fmt.Sprintf("%06d", n.Int64()+100000)
}
```

- [ ] **Шаг 4: Добавить endpoint `VerifyEmail`**

```go
type verifyRequest struct {
	Email string `json:"email" binding:"required,email"`
	Code  string `json:"code" binding:"required"`
}

// POST /api/v1/auth/verify
func (h *AuthHandler) VerifyEmail(c *gin.Context) {
	var req verifyRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid request"})
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

	// Создаём юзера (обрабатываем race condition если email зарегистрировали параллельно)
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
```

> Добавить импорты `"strings"` и `"crypto/subtle"`.

- [ ] **Шаг 5: НЕ коммитить отдельно — NewAuthHandler подпись изменилась**

> **ВАЖНО:** После этого шага `router.go` ещё вызывает `NewAuthHandler(db, cfg.JWTSecret)` со старой сигнатурой — код не компилируется. Закоммитить `auth.go` вместе с `router.go` в Task 9.

---

## Task 6: Auth — сброс пароля

**Файл:** `vpn-backend/internal/handlers/auth.go`

- [ ] **Шаг 1: Добавить `ForgotPassword` endpoint**

```go
type forgotPasswordRequest struct {
	Email string `json:"email" binding:"required,email"`
}

// POST /api/v1/auth/forgot-password
func (h *AuthHandler) ForgotPassword(c *gin.Context) {
	var req forgotPasswordRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid request"})
		return
	}

	// Всегда возвращаем 200 (не раскрываем наличие email)
	var user models.User
	if h.db.Where("email = ?", req.Email).First(&user).Error != nil {
		c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
		return
	}

	// Cooldown 60 секунд
	var recent models.VerificationCode
	cooldown := time.Now().Add(-60 * time.Second)
	if h.db.Where("email = ? AND purpose = ? AND created_at > ? AND used = false", req.Email, "reset", cooldown).
		First(&recent).Error == nil {
		c.JSON(http.StatusOK, gin.H{"message": "if this email is registered, a code will be sent"})
		return
	}

	// Инвалидируем старые коды
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
```

- [ ] **Шаг 2: Добавить `ResetPassword` endpoint**

```go
type resetPasswordRequest struct {
	Email    string `json:"email" binding:"required,email"`
	Code     string `json:"code" binding:"required"`
	Password string `json:"password" binding:"required,min=8"`
}

// POST /api/v1/auth/reset-password
func (h *AuthHandler) ResetPassword(c *gin.Context) {
	var req resetPasswordRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid request"})
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
```

- [ ] **Шаг 3: НЕ коммитить отдельно — закоммитить вместе с router.go в Task 9**

---

## Task 7: Admin — hard delete через Unscoped

**Файл:** `vpn-backend/internal/handlers/admin.go`

GORM по умолчанию делает soft delete (заполняет `deleted_at`). Нужен hard delete.

- [ ] **Шаг 1: Обновить `DeleteUser` — использовать `Unscoped().Delete()`**

В функции `DeleteUser` заменить 4 строки удаления:

```go
// Было:
h.db.Where("user_id = ?", id).Delete(&models.VPNKey{})
h.db.Where("user_id = ?", id).Delete(&models.Subscription{})
h.db.Where("user_id = ?", id).Delete(&models.Payment{})
h.db.Delete(&u)

// Стало:
h.db.Unscoped().Where("user_id = ?", id).Delete(&models.VPNKey{})
h.db.Unscoped().Where("user_id = ?", id).Delete(&models.Subscription{})
h.db.Unscoped().Where("user_id = ?", id).Delete(&models.Payment{})
h.db.Unscoped().Delete(&u)
```

- [ ] **Шаг 2: Commit**

```bash
git add vpn-backend/internal/handlers/admin.go
git commit -m "fix: use hard delete (Unscoped) for user and associated data deletion"
```

---

## Task 8: main.go — panic recovery для горутин

**Файл:** `vpn-backend/cmd/server/main.go`

Если горутина паникует — без recovery весь процесс падает → Docker перезапускает → Caddy теряет соединение.

- [ ] **Шаг 1: Добавить `safeGo` и заменить `go` вызовы**

```go
package main

import (
	"log"
	"time"
	"vpn-backend/internal/backup"
	"vpn-backend/internal/config"
	"vpn-backend/internal/database"
	"vpn-backend/internal/handlers"
	"vpn-backend/internal/models"
	"vpn-backend/internal/router"
)

func safeGo(name string, fn func()) {
	go func() {
		defer func() {
			if r := recover(); r != nil {
				log.Printf("[PANIC] goroutine %s: %v", name, r)
			}
		}()
		fn()
	}()
}

func main() {
	cfg := config.Load()

	db := database.Init(cfg.DBPath)
	database.AutoMigrate(db)

	safeGo("sync-servers", func() { handlers.SyncAllServers(db) })
	safeGo("backup-scheduler", func() { backup.RunScheduler(db, cfg) })
	safeGo("renewal-scheduler", func() {
		handlers.RunAutoRenewalScheduler(db, cfg.YooKassaShopID, cfg.YooKassaKey)
	})
	// Очистка истёкших кодов подтверждения раз в час
	safeGo("code-cleanup", func() {
		for {
			time.Sleep(1 * time.Hour)
			db.Where("expires_at < ? OR used = true", time.Now().Add(-24*time.Hour)).
				Delete(&models.VerificationCode{})
		}
	})

	r := router.New(db, cfg)

	log.Printf("Server starting on :%s", cfg.Port)
	if err := r.Run(":" + cfg.Port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
```

- [ ] **Шаг 2: Commit**

```bash
git add vpn-backend/cmd/server/main.go
git commit -m "fix: add panic recovery for background goroutines"
```

---

## Task 9: Router — CORS, rate limiting, webhook IP whitelist, новые routes

**Файл:** `vpn-backend/internal/router/router.go`

Это финальный шаг — всё подключается здесь.

- [ ] **Шаг 1: Переписать `router.go`**

```go
package router

import (
	"net"
	"net/http"
	"strings"
	"time"
	"vpn-backend/internal/config"
	"vpn-backend/internal/handlers"
	"vpn-backend/internal/middleware"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

// YooKassa IP ranges для проверки webhook
var yooKassaCIDRs = []string{
	"185.71.76.0/27",
	"185.71.77.0/27",
	"77.75.153.0/24",
	"77.75.156.0/24",
}

func isYooKassaIP(ip string) bool {
	parsed := net.ParseIP(ip)
	if parsed == nil {
		return false
	}
	for _, cidr := range yooKassaCIDRs {
		_, network, err := net.ParseCIDR(cidr)
		if err != nil {
			continue
		}
		if network.Contains(parsed) {
			return true
		}
	}
	return false
}

func New(db *gorm.DB, cfg *config.Config) *gin.Engine {
	r := gin.Default()

	// CORS — configurable origins
	allowedOrigins := cfg.AllowedOrigins
	r.Use(func(c *gin.Context) {
		origin := c.GetHeader("Origin")
		if allowedOrigins == "*" {
			c.Header("Access-Control-Allow-Origin", "*")
		} else {
			for _, allowed := range strings.Split(allowedOrigins, ",") {
				if strings.TrimSpace(allowed) == origin {
					c.Header("Access-Control-Allow-Origin", origin)
					break
				}
			}
		}
		// Vary: Origin нужен когда origin-specific (не "*"), чтобы CDN/Caddy не кешировали неправильно
		if allowedOrigins != "*" {
			c.Header("Vary", "Origin")
		}
		c.Header("Access-Control-Allow-Methods", "GET,POST,PUT,PATCH,DELETE,OPTIONS")
		c.Header("Access-Control-Allow-Headers", "Authorization,Content-Type")
		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(204)
			return
		}
		c.Next()
	})

	// Rate limiters
	authLimiter := middleware.NewRateLimiter(10, 1*time.Minute)
	webhookLimiter := middleware.NewRateLimiter(30, 1*time.Minute)

	// Handlers
	authH := handlers.NewAuthHandler(db, cfg)
	userH := handlers.NewUserHandler(db)
	vpnH := handlers.NewVPNHandler(db)
	payH := handlers.NewPaymentHandler(db, cfg.YooKassaShopID, cfg.YooKassaKey)
	adminH := handlers.NewAdminHandler(db, cfg)

	auth := middleware.AuthRequired(cfg.JWTSecret)
	admin := middleware.AdminRequired()

	api := r.Group("/api/v1")
	{
		// Public (rate limited)
		authGroup := api.Group("/auth", middleware.RateLimit(authLimiter))
		{
			authGroup.POST("/register", authH.Register)
			authGroup.POST("/verify", authH.VerifyEmail)
			authGroup.POST("/login", authH.Login)
			authGroup.POST("/refresh", authH.Refresh)
			authGroup.POST("/forgot-password", authH.ForgotPassword)
			authGroup.POST("/reset-password", authH.ResetPassword)
		}

		// Webhook — IP whitelist + rate limit
		api.POST("/payments/webhook",
			middleware.RateLimit(webhookLimiter),
			func(c *gin.Context) {
				if !isYooKassaIP(c.ClientIP()) {
					c.AbortWithStatusJSON(http.StatusForbidden, gin.H{"error": "forbidden"})
					return
				}
				c.Next()
			},
			payH.Webhook,
		)

		// Authenticated
		me := api.Group("/me", auth)
		{
			me.GET("", userH.GetMe)
			me.GET("/subscription", userH.GetSubscription)
			me.PATCH("/subscription/auto-renew", userH.SetAutoRenew)
			me.DELETE("/card", userH.DeleteCard)
			me.GET("/config", vpnH.GetConfig)
			me.POST("/config/revoke", vpnH.RevokeConfig)
		}

		payments := api.Group("/payments", auth)
		{
			payments.POST("/create", payH.CreatePayment)
		}

		// Admin only
		adminGrp := api.Group("/admin", auth, admin)
		{
			adminGrp.GET("/users", adminH.GetUsers)
			adminGrp.POST("/users/:id/upgrade", adminH.UpgradeUser)
			adminGrp.POST("/users/:id/revoke", adminH.RevokeUserKeys)
			adminGrp.POST("/users/:id/set-role", adminH.SetUserRole)
			adminGrp.DELETE("/users/:id", adminH.DeleteUser)
			adminGrp.GET("/servers", adminH.GetServers)
			adminGrp.POST("/servers", adminH.AddServer)
			adminGrp.POST("/servers/:id/toggle", adminH.ToggleServer)
			adminGrp.DELETE("/servers/:id", adminH.DeleteServer)
			adminGrp.GET("/payments", adminH.GetPayments)
			adminGrp.POST("/payments/:id/approve", adminH.ApprovePayment)
			adminGrp.GET("/stats", adminH.GetStats)
			adminGrp.POST("/backup/send", adminH.TriggerBackup)
		}
	}

	// Health check
	r.GET("/health", func(c *gin.Context) {
		c.JSON(200, gin.H{"status": "ok"})
	})

	// Веб-панель администратора
	r.Static("/admin", "./admin")
	r.GET("/", func(c *gin.Context) {
		c.Redirect(302, "/admin/")
	})

	return r
}
```

- [ ] **Шаг 2: Commit — вместе с auth.go (фиксируем сломанную сигнатуру)**

```bash
git add vpn-backend/internal/handlers/auth.go vpn-backend/internal/router/router.go
git commit -m "feat: configurable CORS, rate limiting, YooKassa webhook IP whitelist, new auth routes + email verification"
```

---

## Task 10: Финальная сборка и верификация

- [ ] **Шаг 1: Собрать бэкенд**

```bash
cd vpn-backend
go build ./cmd/server
```

Ожидается: `server` / `server.exe` без ошибок.

- [ ] **Шаг 2: Задеплоить на сервер**

```bash
# На сервере:
docker compose up --build -d vpn-backend
docker compose -f docker-compose.production.yml restart caddy
```

- [ ] **Шаг 3: Проверить**

```bash
curl -k https://srv.frakebit.com/health
# → {"status":"ok"}

curl -k https://srv.frakebit.com/admin/
# → HTML страница входа

curl -k -X POST https://srv.frakebit.com/api/v1/auth/register \
  -H "Content-Type: application/json" \
  -d '{"email":"test@test.com","password":"testpassword123"}'
# → {"message":"verification code sent to email"}
```

- [ ] **Шаг 4: Финальный commit (если всё ок)**

```bash
git add -A
git commit -m "chore: final verification pass"
```

---

## Замечания

- **`NewAuthHandler` сигнатура меняется** — Tasks 5+6 и Task 9 нужно коммитить вместе (auth.go + router.go), иначе код не компилируется между задачами.
- **Cooldown для кодов** — 60 секунд между повторными отправками. Хранится через `created_at` поле GORM.
- **Регистрация без SMTP** — если `SMTP_HOST` не задан, `sendVerificationEmailAsync` логирует ошибку но не крашит. Код сохраняется в БД. Для dev-режима: смотреть код в логах сервера.
- **YooKassa webhook в dev** — IP whitelist заблокирует локальные тесты. Для тестирования отключить middleware или добавить локальный IP.
- **`safeGo` не перезапускает горутины** — если scheduler паникует и восстанавливается, горутина завершается. Это приемлемо: паника в scheduler указывает на баг кода, а не на transient ошибку. Для перезапуска нужен retry loop — это YAGNI для текущего этапа.
