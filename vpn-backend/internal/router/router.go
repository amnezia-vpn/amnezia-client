package router

import (
	"vpn-backend/internal/config"
	"vpn-backend/internal/handlers"
	"vpn-backend/internal/middleware"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

func New(db *gorm.DB, cfg *config.Config) *gin.Engine {
	r := gin.Default()

	// CORS для веб-панели
	r.Use(func(c *gin.Context) {
		c.Header("Access-Control-Allow-Origin", "*")
		c.Header("Access-Control-Allow-Methods", "GET,POST,PUT,DELETE,OPTIONS")
		c.Header("Access-Control-Allow-Headers", "Authorization,Content-Type")
		if c.Request.Method == "OPTIONS" {
			c.AbortWithStatus(204)
			return
		}
		c.Next()
	})

	// Handlers
	authH := handlers.NewAuthHandler(db, cfg.JWTSecret)
	userH := handlers.NewUserHandler(db)
	vpnH := handlers.NewVPNHandler(db)
	payH := handlers.NewPaymentHandler(db, cfg.YooKassaShopID, cfg.YooKassaKey)
	adminH := handlers.NewAdminHandler(db)

	auth := middleware.AuthRequired(cfg.JWTSecret)
	admin := middleware.AdminRequired()

	api := r.Group("/api/v1")
	{
		// Public
		api.POST("/auth/register", authH.Register)
		api.POST("/auth/login", authH.Login)
		api.POST("/auth/refresh", authH.Refresh)
		api.POST("/payments/webhook", payH.Webhook)

		// Authenticated
		me := api.Group("/me", auth)
		{
			me.GET("", userH.GetMe)
			me.GET("/subscription", userH.GetSubscription)
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
			adminGrp.DELETE("/users/:id", adminH.DeleteUser)
			adminGrp.GET("/servers", adminH.GetServers)
			adminGrp.POST("/servers", adminH.AddServer)
			adminGrp.POST("/servers/:id/toggle", adminH.ToggleServer)
			adminGrp.DELETE("/servers/:id", adminH.DeleteServer)
			adminGrp.GET("/payments", adminH.GetPayments)
			adminGrp.POST("/payments/:id/approve", adminH.ApprovePayment)
			adminGrp.GET("/stats", adminH.GetStats)
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
