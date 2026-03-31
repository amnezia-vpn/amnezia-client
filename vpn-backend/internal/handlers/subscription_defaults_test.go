package handlers

import (
	"path/filepath"
	"testing"
	"vpn-backend/internal/models"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
)

func openLegacySubscriptionDB(t *testing.T, schema string) *gorm.DB {
	t.Helper()

	dbPath := filepath.Join(t.TempDir(), "subscriptions-legacy.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}

	if err := db.Exec(schema).Error; err != nil {
		t.Fatalf("create legacy subscriptions: %v", err)
	}

	sqlDB, err := db.DB()
	if err != nil {
		t.Fatalf("db handle: %v", err)
	}
	t.Cleanup(func() {
		_ = sqlDB.Close()
	})

	return db
}

func TestEnsureSubscriptionSchemaAddsExpectedColumn(t *testing.T) {
	db := openLegacySubscriptionDB(t, `
CREATE TABLE subscriptions (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	created_at DATETIME,
	updated_at DATETIME,
	deleted_at DATETIME,
	user_id INTEGER NOT NULL,
	plan TEXT DEFAULT 'free',
	status TEXT DEFAULT 'active',
	expires_at DATETIME,
	auto_renew NUMERIC DEFAULT 1,
	payment_method_id TEXT DEFAULT ''
);`)

	if err := ensureSubscriptionSchema(db); err != nil {
		t.Fatalf("ensureSubscriptionSchema: %v", err)
	}

	if !db.Migrator().HasColumn(&models.Subscription{}, "VIPAdBlockEnabled") {
		t.Fatalf("expected vip_ad_block_enabled column to exist")
	}
}

func TestEnsureSubscriptionSchemaMigratesLegacyVIPColumn(t *testing.T) {
	db := openLegacySubscriptionDB(t, `
CREATE TABLE subscriptions (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	created_at DATETIME,
	updated_at DATETIME,
	deleted_at DATETIME,
	user_id INTEGER NOT NULL,
	plan TEXT DEFAULT 'vip',
	status TEXT DEFAULT 'active',
	expires_at DATETIME,
	auto_renew NUMERIC DEFAULT 1,
	payment_method_id TEXT DEFAULT '',
	v_ip_ad_block_enabled NUMERIC DEFAULT 0
);`)

	if err := db.Exec(`INSERT INTO subscriptions (user_id, plan, status, expires_at, auto_renew, v_ip_ad_block_enabled) VALUES (1, 'vip', 'active', CURRENT_TIMESTAMP, 1, 1)`).Error; err != nil {
		t.Fatalf("insert legacy subscription: %v", err)
	}

	if err := ensureSubscriptionSchema(db); err != nil {
		t.Fatalf("ensureSubscriptionSchema: %v", err)
	}

	var sub models.Subscription
	if err := db.Where("user_id = ?", 1).First(&sub).Error; err != nil {
		t.Fatalf("load migrated subscription: %v", err)
	}

	if !sub.VIPAdBlockEnabled {
		t.Fatalf("expected migrated VIPAdBlockEnabled to be true")
	}
}
