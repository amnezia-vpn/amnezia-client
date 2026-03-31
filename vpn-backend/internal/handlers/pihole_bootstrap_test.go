package handlers

import (
	"path/filepath"
	"strings"
	"testing"
	"time"
	"vpn-backend/internal/models"

	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
)

func openPiHoleTestDB(t *testing.T) *gorm.DB {
	t.Helper()

	dbPath := filepath.Join(t.TempDir(), "pihole-sync.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}

	if err := db.AutoMigrate(&models.VPNServer{}, &models.VLESSServerTemplate{}, &models.Subscription{}); err != nil {
		t.Fatalf("auto migrate: %v", err)
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

func TestBuildPiHoleSeedSQLUsesVIPGroupAndClientBinding(t *testing.T) {
	sql := buildPiHoleSeedSQL("VIP", "172.29.172.2")

	if strings.Contains(sql, "SELECT a.id, 0") {
		t.Fatalf("expected SQL to stop binding canonical lists to Default group")
	}
	if !strings.Contains(sql, `INSERT OR IGNORE INTO client`) {
		t.Fatalf("expected SQL to seed client binding")
	}
	if !strings.Contains(sql, `INSERT OR IGNORE INTO client_by_group`) {
		t.Fatalf("expected SQL to bind client to VIP group")
	}
}

func TestBuildInspectPiHoleDBScriptPrefersGravityAndUsesFilesDatabase(t *testing.T) {
	script := buildInspectPiHoleDBScript()

	if !strings.Contains(script, "pihole-FTL --config files.gravity") {
		t.Fatalf("expected inspect script to look up files.gravity")
	}
	if !strings.Contains(script, "pihole-FTL --config files.database") {
		t.Fatalf("expected inspect script to look up files.database")
	}
	if !strings.Contains(script, "BEST_GRAVITY_DB") {
		t.Fatalf("expected inspect script to prefer gravity db candidates")
	}
}

func TestBuildRecoverPiHoleGravityCommandUsesRecoverFlow(t *testing.T) {
	cmd := buildRecoverPiHoleGravityCommand(piholeHost, "")

	if !strings.Contains(cmd, "pihole -g -r recover") {
		t.Fatalf("expected recovery command to try pihole recover")
	}
	if !strings.Contains(cmd, "pihole updateGravity") {
		t.Fatalf("expected recovery command to try updateGravity fallback")
	}
}

func TestPersistPiHoleSyncResultKeepsLastKnownDNSOnFailure(t *testing.T) {
	db := openPiHoleTestDB(t)

	server := models.VPNServer{
		Name:             "NL1",
		Host:             "138.124.101.69",
		PublicKey:        "pub",
		PiHoleEnabled:    true,
		PiHoleMode:       piholeAuto,
		PiHoleDNSIP:      "172.29.172.1",
		PiHoleLastSyncAt: nil,
	}
	if err := db.Create(&server).Error; err != nil {
		t.Fatalf("create server: %v", err)
	}

	result := piHoleSyncResult{
		ServerID:     server.ID,
		ServerName:   server.Name,
		ModeDetected: piholeHost,
		ClientIP:     "172.29.172.2",
		ErrorCode:    piHoleErrSchemaMismatch,
		Error:        "schema mismatch",
	}
	if err := persistPiHoleSyncResult(db, &server, result); err != nil {
		t.Fatalf("persist result: %v", err)
	}

	var stored models.VPNServer
	if err := db.First(&stored, server.ID).Error; err != nil {
		t.Fatalf("reload server: %v", err)
	}

	if stored.PiHoleDNSIP != "172.29.172.1" {
		t.Fatalf("expected last known good DNS IP to remain, got %q", stored.PiHoleDNSIP)
	}
	if stored.PiHoleLastSyncError == "" {
		t.Fatalf("expected last sync error to be stored")
	}
	if stored.PiHoleLastClientIP != "172.29.172.2" {
		t.Fatalf("expected last client IP to be stored, got %q", stored.PiHoleLastClientIP)
	}
	if stored.PiHoleLastMode != piholeHost {
		t.Fatalf("expected last mode %q, got %q", piholeHost, stored.PiHoleLastMode)
	}
	if stored.PiHoleLastSyncAt == nil || time.Since(*stored.PiHoleLastSyncAt) > time.Minute {
		t.Fatalf("expected recent last sync timestamp, got %v", stored.PiHoleLastSyncAt)
	}
}

func TestResolveVIPDNSConfigUsesHealthyCache(t *testing.T) {
	db := openPiHoleTestDB(t)

	server := models.VPNServer{
		Name:                "NL1",
		Host:                "138.124.101.69",
		PublicKey:           "pub",
		SSHPassword:         "secret",
		PiHoleEnabled:       true,
		PiHoleMode:          piholeAuto,
		PiHoleDNSIP:         "172.29.172.1",
		PiHoleLastMode:      piholeHost,
		PiHoleLastSyncError: "",
	}
	template := &models.VLESSServerTemplate{ContainerName: "amnezia-xray"}
	sub := models.Subscription{VIPAdBlockEnabled: true}

	origSync := syncPiHoleServerFn
	origResolve := resolvePiHoleTargetXrayFn
	t.Cleanup(func() {
		syncPiHoleServerFn = origSync
		resolvePiHoleTargetXrayFn = origResolve
	})

	syncPiHoleServerFn = func(server *models.VPNServer, target string) piHoleSyncResult {
		t.Fatalf("unexpected self-heal sync when cache is healthy")
		return piHoleSyncResult{}
	}
	resolvePiHoleTargetXrayFn = func(server *models.VPNServer, preferred string) string {
		return preferred
	}

	cfg := resolveVIPDNSConfig(db, &server, template, sub)
	if !cfg.Applied {
		t.Fatalf("expected adblock to be applied from cache")
	}
	if cfg.Status != vipAdBlockStatusApplied {
		t.Fatalf("expected status %q, got %q", vipAdBlockStatusApplied, cfg.Status)
	}
	if cfg.Primary != "172.29.172.1" {
		t.Fatalf("expected cached Pi-hole IP, got %q", cfg.Primary)
	}
	if cfg.Source != piHoleDNSSourceHost {
		t.Fatalf("expected source %q, got %q", piHoleDNSSourceHost, cfg.Source)
	}
}

func TestResolveVIPDNSConfigSelfHealsAndPersistsSuccess(t *testing.T) {
	db := openPiHoleTestDB(t)

	server := models.VPNServer{
		Name:          "NL1",
		Host:          "138.124.101.69",
		PublicKey:     "pub",
		SSHPassword:   "secret",
		PiHoleEnabled: true,
		PiHoleMode:    piholeAuto,
	}
	if err := db.Create(&server).Error; err != nil {
		t.Fatalf("create server: %v", err)
	}

	template := &models.VLESSServerTemplate{ContainerName: "amnezia-xray"}
	sub := models.Subscription{VIPAdBlockEnabled: true}

	origSync := syncPiHoleServerFn
	origResolve := resolvePiHoleTargetXrayFn
	t.Cleanup(func() {
		syncPiHoleServerFn = origSync
		resolvePiHoleTargetXrayFn = origResolve
	})

	resolvePiHoleTargetXrayFn = func(server *models.VPNServer, preferred string) string {
		return "amnezia-xray"
	}
	syncPiHoleServerFn = func(server *models.VPNServer, target string) piHoleSyncResult {
		if target != "amnezia-xray" {
			t.Fatalf("expected target container amnezia-xray, got %q", target)
		}
		return piHoleSyncResult{
			ServerID:      server.ID,
			ServerName:    server.Name,
			ModeDetected:  piholeHost,
			DNSIP:         "172.29.172.1",
			ClientIP:      "172.29.172.2",
			ClientSynced:  true,
			GroupSynced:   true,
			ListsSynced:   2,
			XrayReachable: true,
			DNSSource:     piHoleDNSSourceHost,
		}
	}

	cfg := resolveVIPDNSConfig(db, &server, template, sub)
	if !cfg.Applied || cfg.Status != vipAdBlockStatusApplied {
		t.Fatalf("expected self-healed Pi-hole config to be applied, got %+v", cfg)
	}
	if cfg.Primary != "172.29.172.1" {
		t.Fatalf("expected Pi-hole IP, got %q", cfg.Primary)
	}

	var stored models.VPNServer
	if err := db.First(&stored, server.ID).Error; err != nil {
		t.Fatalf("reload server: %v", err)
	}
	if stored.PiHoleDNSIP != "172.29.172.1" {
		t.Fatalf("expected persisted Pi-hole DNS IP, got %q", stored.PiHoleDNSIP)
	}
	if stored.PiHoleLastSyncError != "" {
		t.Fatalf("expected empty last sync error, got %q", stored.PiHoleLastSyncError)
	}
}

func TestResolveVIPDNSConfigReturnsDegradedCleanOnFailedSelfHeal(t *testing.T) {
	db := openPiHoleTestDB(t)

	server := models.VPNServer{
		Name:                "NL1",
		Host:                "138.124.101.69",
		PublicKey:           "pub",
		SSHPassword:         "secret",
		PiHoleEnabled:       true,
		PiHoleMode:          piholeAuto,
		PiHoleDNSIP:         "172.29.172.1",
		PiHoleLastSyncError: "stale cache",
	}
	if err := db.Create(&server).Error; err != nil {
		t.Fatalf("create server: %v", err)
	}

	template := &models.VLESSServerTemplate{ContainerName: "amnezia-xray"}
	sub := models.Subscription{VIPAdBlockEnabled: true}

	origSync := syncPiHoleServerFn
	origResolve := resolvePiHoleTargetXrayFn
	t.Cleanup(func() {
		syncPiHoleServerFn = origSync
		resolvePiHoleTargetXrayFn = origResolve
	})

	resolvePiHoleTargetXrayFn = func(server *models.VPNServer, preferred string) string {
		return "amnezia-xray"
	}
	syncPiHoleServerFn = func(server *models.VPNServer, target string) piHoleSyncResult {
		return piHoleSyncResult{
			ServerID:     server.ID,
			ServerName:   server.Name,
			ModeDetected: piholeHost,
			ErrorCode:    piHoleErrSchemaMismatch,
			Error:        "schema mismatch",
		}
	}

	cfg := resolveVIPDNSConfig(db, &server, template, sub)
	if cfg.Applied {
		t.Fatalf("expected degraded clean DNS config, got applied %+v", cfg)
	}
	if cfg.Status != vipAdBlockStatusDegraded {
		t.Fatalf("expected degraded status, got %q", cfg.Status)
	}
	if cfg.Source != piHoleDNSSourceClean {
		t.Fatalf("expected clean source, got %q", cfg.Source)
	}
	if cfg.Primary != vipDNSPublicPrimary || cfg.Secondary != vipDNSPublicSecondary {
		t.Fatalf("expected clean fallback DNS, got %+v", cfg)
	}

	var stored models.VPNServer
	if err := db.First(&stored, server.ID).Error; err != nil {
		t.Fatalf("reload server: %v", err)
	}
	if stored.PiHoleDNSIP != "172.29.172.1" {
		t.Fatalf("expected last known good DNS IP to remain after failed self-heal, got %q", stored.PiHoleDNSIP)
	}
	if stored.PiHoleLastSyncError == "" {
		t.Fatalf("expected failed self-heal to persist error")
	}
}
