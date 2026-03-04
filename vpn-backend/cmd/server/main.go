package main

import (
	"log"
	"vpn-backend/internal/config"
	"vpn-backend/internal/database"
	"vpn-backend/internal/handlers"
	"vpn-backend/internal/router"
)

func main() {
	cfg := config.Load()

	db := database.Init(cfg.DBPath)
	database.AutoMigrate(db)

	// Автоматическая проверка и синхронизация конфигов серверов
	go handlers.SyncAllServers(db)

	r := router.New(db, cfg)

	log.Printf("Server starting on :%s", cfg.Port)
	if err := r.Run(":" + cfg.Port); err != nil {
		log.Fatalf("Failed to start server: %v", err)
	}
}
