package main

import (
	"crypto/sha256"
	"database/sql"
	"encoding/hex"
	"fmt"
	"log"
	"time"

	_ "modernc.org/sqlite"
)

// --- Data Models ---

type PlanType string

const (
	PlanFreeType PlanType = "free"
	PlanMonthly  PlanType = "monthly"
	PlanYearly   PlanType = "yearly"
)

type SubscriptionStatus string

const (
	StatusActive   SubscriptionStatus = "active"
	StatusExpired  SubscriptionStatus = "expired"
	StatusCanceled SubscriptionStatus = "canceled"
	StatusGrace    SubscriptionStatus = "grace" // 3-day grace period after expiry
)

type User struct {
	ID        string    `json:"id"`
	Email     string    `json:"email"`
	CreatedAt time.Time `json:"createdAt"`
}

type Subscription struct {
	ID          int                `json:"id"`
	UserID      string             `json:"userId"`
	Plan        PlanType           `json:"plan"`
	Status      SubscriptionStatus `json:"status"`
	StartDate   time.Time          `json:"startDate"`
	ExpiryDate  time.Time          `json:"expiryDate"`
	AutoRenew   bool               `json:"autoRenew"`
	LastPayment time.Time          `json:"lastPayment"`
	Price       float64            `json:"price"`
}

type PaymentRecord struct {
	ID        int       `json:"id"`
	UserID    string    `json:"userId"`
	Amount    float64   `json:"amount"`
	Plan      PlanType  `json:"plan"`
	Status    string    `json:"status"` // "success", "failed", "refunded"
	CreatedAt time.Time `json:"createdAt"`
}

// --- Database Layer ---

type SubscriptionDB struct {
	db *sql.DB
}

func NewSubscriptionDB(dbPath string) (*SubscriptionDB, error) {
	db, err := sql.Open("sqlite", dbPath)
	if err != nil {
		return nil, fmt.Errorf("failed to open database: %w", err)
	}

	sdb := &SubscriptionDB{db: db}
	if err := sdb.migrate(); err != nil {
		return nil, fmt.Errorf("failed to migrate database: %w", err)
	}
	return sdb, nil
}

func hashPassword(password string) string {
	h := sha256.Sum256([]byte(password))
	return hex.EncodeToString(h[:])
}

func (s *SubscriptionDB) migrate() error {
	queries := []string{
		`CREATE TABLE IF NOT EXISTS users (
			id TEXT PRIMARY KEY,
			email TEXT NOT NULL UNIQUE,
			password_hash TEXT NOT NULL,
			created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP
		)`,
		`CREATE TABLE IF NOT EXISTS subscriptions (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			user_id TEXT NOT NULL UNIQUE,
			plan TEXT NOT NULL DEFAULT 'free',
			status TEXT NOT NULL DEFAULT 'active',
			start_date DATETIME NOT NULL,
			expiry_date DATETIME NOT NULL,
			auto_renew BOOLEAN NOT NULL DEFAULT 0,
			last_payment DATETIME,
			price REAL NOT NULL DEFAULT 0,
			FOREIGN KEY (user_id) REFERENCES users(id)
		)`,
		`CREATE TABLE IF NOT EXISTS payments (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			user_id TEXT NOT NULL,
			amount REAL NOT NULL,
			plan TEXT NOT NULL,
			status TEXT NOT NULL DEFAULT 'success',
			created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
			FOREIGN KEY (user_id) REFERENCES users(id)
		)`,
		`CREATE TABLE IF NOT EXISTS payment_methods (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			user_id TEXT NOT NULL UNIQUE,
			card_last4 TEXT NOT NULL,
			card_brand TEXT NOT NULL,
			card_expiry TEXT NOT NULL,
			FOREIGN KEY (user_id) REFERENCES users(id)
		)`,
	}
	for _, q := range queries {
		if _, err := s.db.Exec(q); err != nil {
			return err
		}
	}
	return nil
}

func (s *SubscriptionDB) Close() {
	s.db.Close()
}

// --- Auth ---

func (s *SubscriptionDB) Register(email string, password string) (*User, error) {
	id := fmt.Sprintf("user_%x", sha256.Sum256([]byte(email)))[:16]
	hash := hashPassword(password)
	_, err := s.db.Exec(
		`INSERT INTO users (id, email, password_hash) VALUES (?, ?, ?)`,
		id, email, hash,
	)
	if err != nil {
		return nil, fmt.Errorf("registration failed (email may already exist): %w", err)
	}
	// Auto-create free subscription
	_, err = s.CreateFreeSub(id)
	if err != nil {
		return nil, err
	}
	log.Printf("[Auth] New user registered: %s (%s)\n", email, id)
	return &User{ID: id, Email: email, CreatedAt: time.Now()}, nil
}

func (s *SubscriptionDB) Login(email string, password string) (*User, error) {
	hash := hashPassword(password)
	row := s.db.QueryRow(`SELECT id, email, created_at FROM users WHERE email = ? AND password_hash = ?`, email, hash)
	u := &User{}
	err := row.Scan(&u.ID, &u.Email, &u.CreatedAt)
	if err == sql.ErrNoRows {
		return nil, fmt.Errorf("invalid email or password")
	}
	if err != nil {
		return nil, err
	}
	log.Printf("[Auth] User logged in: %s (%s)\n", email, u.ID)
	return u, nil
}

func (s *SubscriptionDB) GetUser(id string) (*User, error) {
	row := s.db.QueryRow(`SELECT id, email, created_at FROM users WHERE id = ?`, id)
	u := &User{}
	err := row.Scan(&u.ID, &u.Email, &u.CreatedAt)
	if err != nil {
		return nil, err
	}
	return u, nil
}

// --- Subscription CRUD ---

func (s *SubscriptionDB) GetSubscription(userID string) (*Subscription, error) {
	row := s.db.QueryRow(
		`SELECT id, user_id, plan, status, start_date, expiry_date, auto_renew, last_payment, price
		 FROM subscriptions WHERE user_id = ?`, userID,
	)
	sub := &Subscription{}
	var lastPayment sql.NullTime
	err := row.Scan(&sub.ID, &sub.UserID, &sub.Plan, &sub.Status,
		&sub.StartDate, &sub.ExpiryDate, &sub.AutoRenew, &lastPayment, &sub.Price)
	if err == sql.ErrNoRows {
		return s.CreateFreeSub(userID)
	}
	if err != nil {
		return nil, err
	}
	if lastPayment.Valid {
		sub.LastPayment = lastPayment.Time
	}
	return sub, nil
}

func (s *SubscriptionDB) CreateFreeSub(userID string) (*Subscription, error) {
	now := time.Now()
	_, err := s.db.Exec(
		`INSERT INTO subscriptions (user_id, plan, status, start_date, expiry_date, auto_renew, price)
		 VALUES (?, 'free', 'active', ?, ?, 0, 0)`,
		userID, now, now.AddDate(100, 0, 0), // Free never expires
	)
	if err != nil {
		return nil, err
	}
	return s.GetSubscription(userID)
}

func (s *SubscriptionDB) UpgradePlan(userID string, plan PlanType) (*Subscription, error) {
	now := time.Now()
	var expiry time.Time
	var price float64

	switch plan {
	case PlanMonthly:
		expiry = now.AddDate(0, 1, 0)
		price = 9.99
	case PlanYearly:
		expiry = now.AddDate(1, 0, 0)
		price = 79.99
	default:
		return nil, fmt.Errorf("invalid plan: %s", plan)
	}

	// Record payment
	_, err := s.db.Exec(
		`INSERT INTO payments (user_id, amount, plan, status) VALUES (?, ?, ?, 'success')`,
		userID, price, plan,
	)
	if err != nil {
		return nil, fmt.Errorf("failed to record payment: %w", err)
	}

	// Update subscription
	_, err = s.db.Exec(
		`UPDATE subscriptions SET plan = ?, status = 'active', start_date = ?, expiry_date = ?,
		 auto_renew = 1, last_payment = ?, price = ? WHERE user_id = ?`,
		plan, now, expiry, now, price, userID,
	)
	if err != nil {
		return nil, fmt.Errorf("failed to upgrade plan: %w", err)
	}

	return s.GetSubscription(userID)
}

func (s *SubscriptionDB) CancelAutoRenew(userID string) error {
	_, err := s.db.Exec(`UPDATE subscriptions SET auto_renew = 0 WHERE user_id = ?`, userID)
	return err
}

func (s *SubscriptionDB) EnableAutoRenew(userID string) error {
	_, err := s.db.Exec(`UPDATE subscriptions SET auto_renew = 1 WHERE user_id = ?`, userID)
	return err
}

// --- Expiration & Auto-Renewal Engine ---

const GracePeriodDays = 3

func (s *SubscriptionDB) CheckAndRenew(userID string) (*Subscription, error) {
	sub, err := s.GetSubscription(userID)
	if err != nil {
		return nil, err
	}

	// Free plan never expires
	if sub.Plan == PlanFreeType {
		return sub, nil
	}

	now := time.Now()

	// Still active
	if now.Before(sub.ExpiryDate) {
		return sub, nil
	}

	// Expired — attempt auto-renew
	if sub.AutoRenew {
		log.Printf("[Subscription] Auto-renewing for user %s (plan: %s)\n", userID, sub.Plan)
		renewed, err := s.UpgradePlan(userID, sub.Plan)
		if err != nil {
			log.Printf("[Subscription] Auto-renew FAILED for user %s: %v\n", userID, err)
			return s.enterGracePeriod(userID, sub)
		}
		log.Printf("[Subscription] Auto-renew SUCCESS for user %s. New expiry: %s\n", userID, renewed.ExpiryDate)
		return renewed, nil
	}

	// No auto-renew, check grace period
	return s.enterGracePeriod(userID, sub)
}

func (s *SubscriptionDB) enterGracePeriod(userID string, sub *Subscription) (*Subscription, error) {
	now := time.Now()
	graceEnd := sub.ExpiryDate.AddDate(0, 0, GracePeriodDays)

	if now.Before(graceEnd) {
		// In grace period
		_, err := s.db.Exec(`UPDATE subscriptions SET status = 'grace' WHERE user_id = ?`, userID)
		if err != nil {
			return nil, err
		}
		sub.Status = StatusGrace
		log.Printf("[Subscription] User %s is in grace period (ends %s)\n", userID, graceEnd)
		return sub, nil
	}

	// Grace period over → downgrade to free
	log.Printf("[Subscription] User %s expired and grace period over. Downgrading to Free.\n", userID)
	_, err := s.db.Exec(
		`UPDATE subscriptions SET plan = 'free', status = 'active', price = 0, auto_renew = 0,
		 expiry_date = ? WHERE user_id = ?`,
		now.AddDate(100, 0, 0), userID,
	)
	if err != nil {
		return nil, err
	}
	return s.GetSubscription(userID)
}

// --- Payment History ---

func (s *SubscriptionDB) GetPaymentHistory(userID string) ([]PaymentRecord, error) {
	rows, err := s.db.Query(
		`SELECT id, user_id, amount, plan, status, created_at FROM payments WHERE user_id = ? ORDER BY created_at DESC LIMIT 20`,
		userID,
	)
	if err != nil {
		return nil, err
	}
	defer rows.Close()

	var records []PaymentRecord
	for rows.Next() {
		var r PaymentRecord
		if err := rows.Scan(&r.ID, &r.UserID, &r.Amount, &r.Plan, &r.Status, &r.CreatedAt); err != nil {
			return nil, err
		}
		records = append(records, r)
	}
	return records, nil
}

// --- Payment Method ---

type PaymentMethod struct {
	CardLast4  string `json:"cardLast4"`
	CardBrand  string `json:"cardBrand"`
	CardExpiry string `json:"cardExpiry"`
}

func (s *SubscriptionDB) SavePaymentMethod(userID string, pm PaymentMethod) error {
	_, err := s.db.Exec(
		`INSERT OR REPLACE INTO payment_methods (user_id, card_last4, card_brand, card_expiry) VALUES (?, ?, ?, ?)`,
		userID, pm.CardLast4, pm.CardBrand, pm.CardExpiry,
	)
	return err
}

func (s *SubscriptionDB) GetPaymentMethod(userID string) (*PaymentMethod, error) {
	row := s.db.QueryRow(`SELECT card_last4, card_brand, card_expiry FROM payment_methods WHERE user_id = ?`, userID)
	pm := &PaymentMethod{}
	err := row.Scan(&pm.CardLast4, &pm.CardBrand, &pm.CardExpiry)
	if err == sql.ErrNoRows {
		return nil, nil
	}
	return pm, err
}
