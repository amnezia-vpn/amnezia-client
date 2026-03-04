package config

import (
	"log"
	"os"

	"github.com/joho/godotenv"
)

type Config struct {
	Port           string
	DBPath         string
	JWTSecret      string
	RedisAddr      string
	YooKassaShopID string
	YooKassaKey    string
}

func Load() *Config {
	if err := godotenv.Load(); err != nil {
		log.Println("No .env file found, using environment variables")
	}

	return &Config{
		Port:           getEnv("PORT", "8080"),
		DBPath:         getEnv("DB_PATH", "data/vpn.db"),
		JWTSecret:      getEnv("JWT_SECRET", "change-me-in-production"),
		RedisAddr:      getEnv("REDIS_ADDR", "localhost:6379"),
		YooKassaShopID: getEnv("YOOKASSA_SHOP_ID", ""),
		YooKassaKey:    getEnv("YOOKASSA_SECRET_KEY", ""),
	}
}

func getEnv(key, defaultValue string) string {
	if val := os.Getenv(key); val != "" {
		return val
	}
	return defaultValue
}
