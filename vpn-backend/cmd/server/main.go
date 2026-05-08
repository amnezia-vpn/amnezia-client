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
			db.Where("expires_at < ? OR status IN ?", time.Now().Add(-24*time.Hour),
				[]models.TVLoginStatus{models.TVLoginConsumed, models.TVLoginExpired}).
				Delete(&models.TVLogin{})
		}
	})

	r := router.New(db, cfg)

	log.Printf("Server starting on :%s", cfg.Port)
	if err := r.Run(":" + cfg.Port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
