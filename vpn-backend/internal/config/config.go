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

	// Платежи
	PaymentReturnURL string // URL для редиректа после оплаты

	// Client Updater
	ClientLatestVersion  string
	ClientDownloadURL    string
	ClientReleaseNotes   string
	ClientUpdateCritical bool
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
		PaymentReturnURL:    getEnv("PAYMENT_RETURN_URL", "https://frakebit.com/payment/success"),

		ClientLatestVersion:  getEnv("CLIENT_LATEST_VERSION", "1.0.0"),
		ClientDownloadURL:    getEnv("CLIENT_DOWNLOAD_URL", "https://frakebit.com/download"),
		ClientReleaseNotes:   getEnv("CLIENT_RELEASE_NOTES", "Улучшена стабильность и скорость."),
		ClientUpdateCritical: getEnv("CLIENT_UPDATE_CRITICAL", "false") == "true",
	}
}

func getEnv(key, defaultValue string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultValue
}
