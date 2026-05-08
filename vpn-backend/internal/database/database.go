package database

import (
	"log"
	"os"
	"path/filepath"
	"vpn-backend/internal/models"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
	"gorm.io/gorm/logger"
)

func Init(dbPath string) *gorm.DB {
	// Ensure directory exists
	if err := os.MkdirAll(filepath.Dir(dbPath), 0755); err != nil {
		log.Fatalf("Failed to create db directory: %v", err)
	}

	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{
		Logger: logger.Default.LogMode(logger.Info),
	})
	if err != nil {
		log.Fatalf("Failed to connect to database: %v", err)
	}

	sqlDB, err := db.DB()
	if err != nil {
		log.Fatalf("Failed to get sql.DB handle: %v", err)
	}

	// SQLite is reliable here, but it needs conservative pooling plus WAL/busy_timeout
	// to avoid SQLITE_BUSY under concurrent auth + subscription requests.
	sqlDB.SetMaxOpenConns(1)
	sqlDB.SetMaxIdleConns(1)

	if err := db.Exec("PRAGMA journal_mode = WAL;").Error; err != nil {
		log.Printf("WARN: failed to enable WAL mode: %v", err)
	}
	if err := db.Exec("PRAGMA busy_timeout = 15000;").Error; err != nil {
		log.Printf("WARN: failed to set busy_timeout: %v", err)
	}
	if err := db.Exec("PRAGMA synchronous = NORMAL;").Error; err != nil {
		log.Printf("WARN: failed to set synchronous pragma: %v", err)
	}
	if err := db.Exec("PRAGMA foreign_keys = ON;").Error; err != nil {
		log.Printf("WARN: failed to enable foreign keys: %v", err)
	}

	return db
}

func AutoMigrate(db *gorm.DB) {
	err := db.AutoMigrate(
		&models.User{},
		&models.Subscription{},
		&models.TVLogin{},
		&models.VPNServer{},
		&models.VPNKey{},
		&models.VLESSServerTemplate{},
		&models.VLESSCredential{},
		&models.RoutingProfile{},
		&models.PromoCode{},
		&models.Payment{},
		&models.VerificationCode{},
	)
	if err != nil {
		log.Fatalf("Failed to migrate database: %v", err)
	}
	log.Println("Database migrated successfully")
}
