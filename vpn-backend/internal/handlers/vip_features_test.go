package handlers

import (
	"path/filepath"
	"testing"
	"vpn-backend/internal/models"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
)

func openLegacyRoutingProfileDB(t *testing.T) *gorm.DB {
	t.Helper()

	dbPath := filepath.Join(t.TempDir(), "routing-profiles-legacy.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}

	legacySchema := `
CREATE TABLE routing_profiles (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	created_at DATETIME,
	updated_at DATETIME,
	deleted_at DATETIME,
	user_id INTEGER NOT NULL,
	name TEXT NOT NULL,
	kind TEXT DEFAULT 'custom',
	enabled NUMERIC DEFAULT 0,
	domains_json TEXT DEFAULT '[]',
	domain_suffixes_json TEXT DEFAULT '[]',
	cidrs_json TEXT DEFAULT '[]'
);
`
	if err := db.Exec(legacySchema).Error; err != nil {
		t.Fatalf("create legacy routing_profiles: %v", err)
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

func TestEnsureRoutingProfileSchemaUpgradesLegacyTable(t *testing.T) {
	db := openLegacyRoutingProfileDB(t)

	if err := ensureRoutingProfileSchema(db); err != nil {
		t.Fatalf("ensureRoutingProfileSchema: %v", err)
	}

	for _, field := range []string{"Code", "Action", "Description", "Icon", "SortOrder"} {
		if !db.Migrator().HasColumn(&models.RoutingProfile{}, field) {
			t.Fatalf("expected column %s to exist after schema fix", field)
		}
	}
}

func TestEnsureDefaultRoutingProfilesHandlesLegacyRows(t *testing.T) {
	db := openLegacyRoutingProfileDB(t)

	insertLegacy := `
INSERT INTO routing_profiles (user_id, name, kind, enabled, domains_json, domain_suffixes_json, cidrs_json)
VALUES (1, 'RU without VPN', 'system', 1, '[]', '[".ru"]', '[]');
`
	if err := db.Exec(insertLegacy).Error; err != nil {
		t.Fatalf("insert legacy row: %v", err)
	}

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles: %v", err)
	}

	var profile models.RoutingProfile
	if err := db.Where("user_id = ? AND code = ?", 1, "ru_direct").First(&profile).Error; err != nil {
		t.Fatalf("load upgraded ru_direct profile: %v", err)
	}

	if profile.Action != models.RoutingProfileDirect {
		t.Fatalf("expected action %q, got %q", models.RoutingProfileDirect, profile.Action)
	}

	if profile.Description == "" {
		t.Fatalf("expected seeded description to be populated")
	}
}
