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
	userH := handlers.NewUserHandler(db, cfg)
	vpnH := handlers.NewVPNHandler(db)
	payH := handlers.NewPaymentHandler(db, cfg.YooKassaShopID, cfg.YooKassaKey, cfg)
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
			adminGrp.PUT("/servers/:id", adminH.UpdateServer)
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
