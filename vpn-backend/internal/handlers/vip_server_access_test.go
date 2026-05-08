package handlers

import (
	"encoding/json"
	"fmt"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strings"
	"testing"
	"time"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
)

func openVIPServerAccessDB(t *testing.T) *gorm.DB {
	t.Helper()

	dbPath := filepath.Join(t.TempDir(), "vip-server-access.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}
	if err := db.AutoMigrate(
		&models.User{},
		&models.Subscription{},
		&models.VPNServer{},
		&models.VPNKey{},
		&models.VLESSServerTemplate{},
		&models.VLESSCredential{},
		&models.RoutingProfile{},
	).Error; err != nil {
		t.Fatalf("migrate: %v", err)
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

func seedVIPAccessSubscription(t *testing.T, db *gorm.DB, userID uint, plan models.PlanType) {
	t.Helper()

	if err := db.Create(&models.User{
		Model:        gorm.Model{ID: userID},
		Email:        "user@example.com",
		PasswordHash: "hash",
	}).Error; err != nil {
		t.Fatalf("create user: %v", err)
	}
	if err := db.Create(&models.Subscription{
		UserID:    userID,
		Plan:      plan,
		Status:    models.SubActive,
		ExpiresAt: time.Now().Add(24 * time.Hour),
	}).Error; err != nil {
		t.Fatalf("create subscription: %v", err)
	}
}

func seedVIPAccessServer(t *testing.T, db *gorm.DB, name string, vipOnly bool) models.VPNServer {
	t.Helper()

	server := models.VPNServer{
		Name:        name,
		Host:        strings.ToLower(name) + ".example.com",
		PublicKey:   "server-public-key-" + name,
		Region:      name,
		CountryCode: "NL",
		Active:      true,
		VIPOnly:     vipOnly,
		H1:          "1",
		H2:          "2",
		H3:          "3",
		H4:          "4",
	}
	if err := db.Create(&server).Error; err != nil {
		t.Fatalf("create server: %v", err)
	}
	return server
}

func seedVIPAccessVLESSTemplate(t *testing.T, db *gorm.DB, server models.VPNServer) {
	t.Helper()

	if err := db.Create(&models.VLESSServerTemplate{
		ServerID:    server.ID,
		Address:     server.Host,
		Port:        443,
		ServerName:  "www.googletagmanager.com",
		PublicKey:   "reality-public-key-" + server.Name,
		ShortID:     "0123456789abcdef",
		Fingerprint: "chrome",
		Flow:        "xtls-rprx-vision",
		Network:     "tcp",
		Security:    "reality",
	}).Error; err != nil {
		t.Fatalf("create vless template: %v", err)
	}
}

func TestVPNServerVIPOnlyDefaultsFalse(t *testing.T) {
	db := openVIPServerAccessDB(t)

	server := models.VPNServer{
		Name:      "Default",
		Host:      "default.example.com",
		PublicKey: "server-public-key-default",
		Active:    true,
	}
	if err := db.Create(&server).Error; err != nil {
		t.Fatalf("create server: %v", err)
	}

	var stored models.VPNServer
	if err := db.First(&stored, server.ID).Error; err != nil {
		t.Fatalf("load server: %v", err)
	}
	if stored.VIPOnly {
		t.Fatalf("expected vip_only to default to false")
	}
}

func TestAdminCreateUpdateVIPOnlyServer(t *testing.T) {
	db := openVIPServerAccessDB(t)
	handler := NewAdminHandler(db, nil)
	gin.SetMode(gin.TestMode)

	createBody := strings.NewReader(`{"name":"VIP","host":"vip.example.com","public_key":"pub","is_vip_only":true}`)
	createRecorder := httptest.NewRecorder()
	createContext, _ := gin.CreateTestContext(createRecorder)
	createContext.Request = httptest.NewRequest(http.MethodPost, "/api/v1/admin/servers", createBody)
	createContext.Request.Header.Set("Content-Type", "application/json")
	handler.AddServer(createContext)
	if createRecorder.Code != http.StatusCreated {
		t.Fatalf("expected create status 201, got %d body=%s", createRecorder.Code, createRecorder.Body.String())
	}

	var server models.VPNServer
	if err := db.Where("name = ?", "VIP").First(&server).Error; err != nil {
		t.Fatalf("load created server: %v", err)
	}
	if !server.VIPOnly {
		t.Fatalf("expected created server to be vip-only")
	}

	updateRecorder := httptest.NewRecorder()
	updateContext, _ := gin.CreateTestContext(updateRecorder)
	updateContext.Params = gin.Params{{Key: "id", Value: fmt.Sprint(server.ID)}}
	updateContext.Request = httptest.NewRequest(http.MethodPut, "/api/v1/admin/servers/"+fmt.Sprint(server.ID), strings.NewReader(`{"country_code":"NL","is_vip_only":false}`))
	updateContext.Request.Header.Set("Content-Type", "application/json")
	handler.UpdateServer(updateContext)
	if updateRecorder.Code != http.StatusOK {
		t.Fatalf("expected update status 200, got %d body=%s", updateRecorder.Code, updateRecorder.Body.String())
	}

	if err := db.First(&server, server.ID).Error; err != nil {
		t.Fatalf("reload server: %v", err)
	}
	if server.VIPOnly {
		t.Fatalf("expected update to disable vip-only")
	}
}

func TestGetConfigFiltersVIPOnlyServersForBasicUsers(t *testing.T) {
	db := openVIPServerAccessDB(t)
	seedVIPAccessSubscription(t, db, 1, models.PlanBasic)
	normalServer := seedVIPAccessServer(t, db, "Normal", false)
	vipServer := seedVIPAccessServer(t, db, "VIP", true)

	handler := NewVPNHandler(db)
	recorder := httptest.NewRecorder()
	context, _ := gin.CreateTestContext(recorder)
	context.Set("user_id", uint(1))
	context.Request = httptest.NewRequest(http.MethodGet, "/api/v1/me/config", nil)
	handler.GetConfig(context)

	if recorder.Code != http.StatusOK {
		t.Fatalf("expected config status 200, got %d body=%s", recorder.Code, recorder.Body.String())
	}

	var response struct {
		Config string `json:"config"`
	}
	if err := json.Unmarshal(recorder.Body.Bytes(), &response); err != nil {
		t.Fatalf("decode response: %v", err)
	}
	if strings.Contains(response.Config, vipServer.Host) || strings.Contains(response.Config, `"server_id":`+jsonNumber(vipServer.ID)) {
		t.Fatalf("basic config leaked vip-only server: %s", response.Config)
	}
	if !strings.Contains(response.Config, normalServer.Host) {
		t.Fatalf("basic config did not include normal server: %s", response.Config)
	}

	var keys []models.VPNKey
	if err := db.Find(&keys).Error; err != nil {
		t.Fatalf("load keys: %v", err)
	}
	if len(keys) != 1 || keys[0].ServerID != normalServer.ID {
		t.Fatalf("expected exactly one key for normal server, got %+v", keys)
	}
}

func TestGetConfigIncludesVIPOnlyServersForVIPUsers(t *testing.T) {
	db := openVIPServerAccessDB(t)
	seedVIPAccessSubscription(t, db, 1, models.PlanVIP)
	normalServer := seedVIPAccessServer(t, db, "Normal", false)
	vipServer := seedVIPAccessServer(t, db, "VIP", true)
	seedVIPAccessVLESSTemplate(t, db, normalServer)
	seedVIPAccessVLESSTemplate(t, db, vipServer)

	handler := NewVPNHandler(db)
	recorder := httptest.NewRecorder()
	context, _ := gin.CreateTestContext(recorder)
	context.Set("user_id", uint(1))
	context.Request = httptest.NewRequest(http.MethodGet, "/api/v1/me/config", nil)
	handler.GetConfig(context)

	if recorder.Code != http.StatusOK {
		t.Fatalf("expected config status 200, got %d body=%s", recorder.Code, recorder.Body.String())
	}

	body := recorder.Body.String()
	if !strings.Contains(body, normalServer.Host) || !strings.Contains(body, vipServer.Host) {
		t.Fatalf("expected VIP config to include normal and VIP-only servers, got %s", body)
	}

	var credentials []models.VLESSCredential
	if err := db.Order("server_id asc").Find(&credentials).Error; err != nil {
		t.Fatalf("load vless credentials: %v", err)
	}
	if len(credentials) != 2 {
		t.Fatalf("expected credentials for both servers, got %+v", credentials)
	}
}

func TestGetServersReturnsSafeVIPOnlyMetadata(t *testing.T) {
	db := openVIPServerAccessDB(t)
	seedVIPAccessSubscription(t, db, 1, models.PlanBasic)
	seedVIPAccessServer(t, db, "Normal", false)
	vipServer := seedVIPAccessServer(t, db, "VIP", true)

	handler := NewUserHandler(db, nil)
	recorder := httptest.NewRecorder()
	context, _ := gin.CreateTestContext(recorder)
	context.Set("user_id", uint(1))
	context.Request = httptest.NewRequest(http.MethodGet, "/api/v1/me/servers", nil)
	handler.GetServers(context)

	if recorder.Code != http.StatusOK {
		t.Fatalf("expected metadata status 200, got %d body=%s", recorder.Code, recorder.Body.String())
	}
	if strings.Contains(recorder.Body.String(), vipServer.PublicKey) {
		t.Fatalf("metadata leaked server public key: %s", recorder.Body.String())
	}

	var response struct {
		Servers []struct {
			ID          uint   `json:"id"`
			IsVIPOnly   bool   `json:"is_vip_only"`
			IsAvailable bool   `json:"is_available"`
			HostName    string `json:"host_name"`
		} `json:"servers"`
	}
	if err := json.Unmarshal(recorder.Body.Bytes(), &response); err != nil {
		t.Fatalf("decode metadata response: %v", err)
	}

	foundLockedVIP := false
	for _, server := range response.Servers {
		if server.ID == vipServer.ID {
			foundLockedVIP = server.IsVIPOnly && !server.IsAvailable && server.HostName != ""
		}
	}
	if !foundLockedVIP {
		t.Fatalf("expected locked vip-only metadata, got %+v", response.Servers)
	}
}

func jsonNumber(id uint) string {
	data, _ := json.Marshal(id)
	return string(data)
}
