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
	RedisAddr      string
	YooKassaShopID string
	YooKassaKey    string

	// SMTP для бэкапов
	SMTPHost            string
	SMTPPort            int
	SMTPUser            string
	SMTPPassword        string
	SMTPFrom            string // если пусто — берётся SMTPUser
	BackupIntervalHours int    // интервал бэкапа в часах (по умолчанию 24)
}

func Load() *Config {
	if err := godotenv.Load(); err != nil {
		log.Println("No .env file found, using environment variables")
	}

	smtpPort, _ := strconv.Atoi(getEnv("SMTP_PORT", "587"))
	backupInterval, _ := strconv.Atoi(getEnv("BACKUP_INTERVAL_HOURS", "24"))
	if backupInterval <= 0 {
		backupInterval = 24
	}

	return &Config{
		Port:           getEnv("PORT", "8080"),
		DBPath:         getEnv("DB_PATH", "data/vpn.db"),
		JWTSecret:      getEnv("JWT_SECRET", "change-me-in-production"),
		RedisAddr:      getEnv("REDIS_ADDR", "localhost:6379"),
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
