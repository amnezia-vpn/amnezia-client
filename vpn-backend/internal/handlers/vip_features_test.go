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

	for _, field := range []string{"Code", "TemplateCode", "Action", "Description", "Icon", "SortOrder"} {
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

func TestEnsureDefaultRoutingProfilesMigratesEnabledSystemToCustomCopy(t *testing.T) {
	db := openLegacyRoutingProfileDB(t)

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles: %v", err)
	}

	var systemProfile models.RoutingProfile
	if err := db.Where("user_id = ? AND code = ?", 1, "ai_proxy").First(&systemProfile).Error; err != nil {
		t.Fatalf("load ai_proxy system profile: %v", err)
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("id = ?", systemProfile.ID).
		Update("enabled", true).Error; err != nil {
		t.Fatalf("enable system profile: %v", err)
	}

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles after enable: %v", err)
	}

	var customCopies []models.RoutingProfile
	if err := db.Where("user_id = ? AND kind = ? AND template_code = ?", 1, models.RoutingProfileCustom, "ai_proxy").
		Find(&customCopies).Error; err != nil {
		t.Fatalf("load custom copies: %v", err)
	}

	if len(customCopies) != 1 {
		t.Fatalf("expected exactly one custom copy, got %d", len(customCopies))
	}
	if !customCopies[0].Enabled {
		t.Fatalf("expected custom copy to be enabled")
	}

	var systemAfter models.RoutingProfile
	if err := db.First(&systemAfter, systemProfile.ID).Error; err != nil {
		t.Fatalf("reload system profile: %v", err)
	}
	if systemAfter.Enabled {
		t.Fatalf("expected system profile to be disabled after migration")
	}

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles idempotency: %v", err)
	}
	var customCount int64
	if err := db.Model(&models.RoutingProfile{}).
		Where("user_id = ? AND kind = ? AND template_code = ?", 1, models.RoutingProfileCustom, "ai_proxy").
		Count(&customCount).Error; err != nil {
		t.Fatalf("count custom copies: %v", err)
	}
	if customCount != 1 {
		t.Fatalf("expected idempotent single custom copy, got %d", customCount)
	}
}

func TestEnsureCustomRoutingProfileFromTemplateIsIdempotent(t *testing.T) {
	db := openLegacyRoutingProfileDB(t)

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles: %v", err)
	}

	var systemProfile models.RoutingProfile
	if err := db.Where("user_id = ? AND code = ?", 1, "media_proxy").First(&systemProfile).Error; err != nil {
		t.Fatalf("load media_proxy system profile: %v", err)
	}

	first, created, err := ensureCustomRoutingProfileFromTemplate(db, 1, systemProfile, true)
	if err != nil {
		t.Fatalf("first ensureCustomRoutingProfileFromTemplate: %v", err)
	}
	if !created {
		t.Fatalf("expected first copy call to create profile")
	}

	second, created, err := ensureCustomRoutingProfileFromTemplate(db, 1, systemProfile, true)
	if err != nil {
		t.Fatalf("second ensureCustomRoutingProfileFromTemplate: %v", err)
	}
	if created {
		t.Fatalf("expected second copy call to reuse existing profile")
	}
	if first.ID != second.ID {
		t.Fatalf("expected same profile id for repeated copy, got %d and %d", first.ID, second.ID)
	}
}

func TestEnsureDefaultRoutingProfilesEnablesExistingCustomCopyDuringMigration(t *testing.T) {
	db := openLegacyRoutingProfileDB(t)

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles: %v", err)
	}

	var systemProfile models.RoutingProfile
	if err := db.Where("user_id = ? AND code = ?", 1, "ai_proxy").First(&systemProfile).Error; err != nil {
		t.Fatalf("load ai_proxy system profile: %v", err)
	}

	custom, created, err := ensureCustomRoutingProfileFromTemplate(db, 1, systemProfile, false)
	if err != nil {
		t.Fatalf("create custom copy disabled: %v", err)
	}
	if !created {
		t.Fatalf("expected custom copy to be created")
	}
	if custom.Enabled {
		t.Fatalf("expected custom copy to be disabled initially")
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("id = ?", systemProfile.ID).
		Update("enabled", true).Error; err != nil {
		t.Fatalf("enable system profile: %v", err)
	}

	if err := ensureDefaultRoutingProfiles(db, 1); err != nil {
		t.Fatalf("ensureDefaultRoutingProfiles migration: %v", err)
	}

	var customAfter models.RoutingProfile
	if err := db.Where("id = ?", custom.ID).First(&customAfter).Error; err != nil {
		t.Fatalf("reload custom profile: %v", err)
	}
	if !customAfter.Enabled {
		t.Fatalf("expected migration to enable existing custom copy")
	}
}
