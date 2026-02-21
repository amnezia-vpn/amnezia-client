package main

import (
	"database/sql"
	"encoding/json"
	"log"
	"net/http"
	"os"

	_ "modernc.org/sqlite"
)

// Config structure
type Config struct {
	Port              string
	YookassaShopID    string
	YookassaSecretKey string
	YookassaReturnURL string
}

type Server struct {
	DB       *sql.DB
	Cfg      *Config
	YooKassa *YooKassaClient
}

func main() {
	// Initialize Config
	cfg := LoadConfig()

	// Initialize DB (supports DB_PATH env var for Docker)
	dbPath := os.Getenv("DB_PATH")
	if dbPath == "" {
		dbPath = "server.db"
	}
	db, err := sql.Open("sqlite", dbPath)
	if err != nil {
		log.Fatal(err)
	}
	log.Printf("Database: %s", dbPath)
	defer db.Close()

	// Enable WAL mode to prevent Database is Locked errors
	_, err = db.Exec("PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA synchronous=NORMAL;")
	if err != nil {
		log.Printf("Warning: Failed to set WAL mode: %v", err)
	}

	// Create tables
	initDB(db)

	srv := &Server{
		DB:       db,
		Cfg:      cfg,
		YooKassa: NewYooKassaClient(cfg.YookassaShopID, cfg.YookassaSecretKey),
	}

	// Router
	mux := http.NewServeMux()
	mux.HandleFunc("/register", srv.handleRegister)
	mux.HandleFunc("/login", srv.handleLogin)
	mux.HandleFunc("/servers", srv.handleGetServers)
	mux.HandleFunc("/payment/init", srv.handleInitPayment)
	mux.HandleFunc("/payment/check", srv.handleCheckPayment)
	mux.HandleFunc("/payment/webhook", srv.handleWebhook)
	mux.HandleFunc("/admin/add-server", srv.handleAdminAddServer)
	mux.HandleFunc("/health", func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK)
		w.Write([]byte("OK"))
	})

	log.Printf("Server starting on %s...", cfg.Port)
	log.Fatal(http.ListenAndServe(cfg.Port, mux))
}

func LoadConfig() *Config {
	cfg := &Config{}

	// Try loading from config.json first
	configPath := os.Getenv("CONFIG_PATH")
	if configPath == "" {
		configPath = "config.json"
	}
	file, err := os.Open(configPath)
	if err == nil {
		defer file.Close()
		if decErr := json.NewDecoder(file).Decode(cfg); decErr != nil {
			log.Printf("Error decoding %s: %v", configPath, decErr)
		}
	}

	// Environment variables ALWAYS override config.json
	if v := os.Getenv("PORT"); v != "" {
		cfg.Port = v
	}
	if v := os.Getenv("YOOKASSA_SHOP_ID"); v != "" {
		cfg.YookassaShopID = v
	}
	if v := os.Getenv("YOOKASSA_SECRET_KEY"); v != "" {
		cfg.YookassaSecretKey = v
	}
	if v := os.Getenv("YOOKASSA_RETURN_URL"); v != "" {
		cfg.YookassaReturnURL = v
	}

	// Defaults
	if cfg.Port == "" {
		cfg.Port = ":8080"
	}
	if cfg.YookassaReturnURL == "" {
		cfg.YookassaReturnURL = "https://google.com"
	}

	return cfg
}

func initDB(db *sql.DB) {
	queries := []string{
		`CREATE TABLE IF NOT EXISTS users (
			id TEXT PRIMARY KEY,
			email TEXT UNIQUE,
			password TEXT,
			plan TEXT,
			expiry_date DATETIME,
			created_at DATETIME DEFAULT CURRENT_TIMESTAMP
		);`,
		`CREATE TABLE IF NOT EXISTS payments (
			id TEXT PRIMARY KEY,
			user_id TEXT,
			yookassa_id TEXT,
			amount REAL,
			status TEXT,
			created_at DATETIME DEFAULT CURRENT_TIMESTAMP
		);`,
		`CREATE TABLE IF NOT EXISTS servers (
			id TEXT PRIMARY KEY,
			country TEXT,
			city TEXT,
			flag TEXT,
			is_premium BOOLEAN,
			type TEXT DEFAULT 'amnezia',
			server_host TEXT DEFAULT '',
			ssh_user TEXT DEFAULT '',
			ssh_pass TEXT DEFAULT '',
			settings TEXT DEFAULT '{}'
		);`,
		`CREATE TABLE IF NOT EXISTS access_keys (
			user_id TEXT,
			server_id TEXT,
			key_id TEXT,
			access_url TEXT,
			PRIMARY KEY (user_id, server_id),
			FOREIGN KEY(user_id) REFERENCES users(id),
			FOREIGN KEY(server_id) REFERENCES servers(id)
		);`,
	}

	for _, q := range queries {
		if _, err := db.Exec(q); err != nil {
			log.Printf("Error creating table: %v", err)
		}
	}

	// Migrations for existing databases
	migrations := []string{
		`ALTER TABLE servers ADD COLUMN type TEXT DEFAULT 'amnezia';`,
		`ALTER TABLE servers ADD COLUMN server_host TEXT DEFAULT '';`,
		`ALTER TABLE servers ADD COLUMN ssh_user TEXT DEFAULT '';`,
		`ALTER TABLE servers ADD COLUMN ssh_pass TEXT DEFAULT '';`,
		`ALTER TABLE servers ADD COLUMN settings TEXT DEFAULT '{}';`,
	}
	for _, m := range migrations {
		db.Exec(m) // Ignore errors (column already exists)
	}
}
