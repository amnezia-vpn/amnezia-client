package handlers

import (
	"net"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"testing"
	"time"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/glebarez/sqlite"
	"gorm.io/gorm"
)

func TestRevokeConfigPerformance(t *testing.T) {
	// Start a dummy TCP server to simulate a slow SSH server
	ln, err := net.Listen("tcp", "127.0.0.1:0")
	if err != nil {
		t.Fatalf("failed to listen: %v", err)
	}
	defer ln.Close()

	go func() {
		for {
			conn, err := ln.Accept()
			if err != nil {
				return
			}
			go func(c net.Conn) {
				defer c.Close()
				time.Sleep(500 * time.Millisecond) // Simulate slow connection
			}(conn)
		}
	}()

	// Setup DB
	dbPath := filepath.Join(t.TempDir(), "perf.db")
	db, err := gorm.Open(sqlite.Open(dbPath), &gorm.Config{})
	if err != nil {
		t.Fatalf("open sqlite: %v", err)
	}
	db.AutoMigrate(&models.VPNServer{}, &models.VLESSServerTemplate{}, &models.VPNKey{}, &models.VLESSCredential{})

	handler := &VPNHandler{db: db}

	// Create test user (ID=1) and 5 slow connections
	userID := uint(1)
	numItems := 5

	for i := 0; i < numItems; i++ {
		server := models.VPNServer{
			Name:        "TestServer",
			Host:        "127.0.0.1",
			SSHHost:     ln.Addr().(*net.TCPAddr).IP.String(),
			SSHPort:     ln.Addr().(*net.TCPAddr).Port,
			SSHUser:     "root",
			SSHPassword: "password", // trigger SSH connection
		}
		db.Create(&server)

		template := models.VLESSServerTemplate{
			ServerID: server.ID,
		}
		db.Create(&template)

		key := models.VPNKey{
			UserID:    userID,
			ServerID:  server.ID,
			PublicKey: "dummy-pub-key",
		}
		db.Create(&key)

		cred := models.VLESSCredential{
			UserID:   userID,
			ServerID: server.ID,
			ClientID: "dummy-client-id",
		}
		db.Create(&cred)
	}

	gin.SetMode(gin.TestMode)
	w := httptest.NewRecorder()
	c, _ := gin.CreateTestContext(w)
	c.Set("user_id", userID)
	c.Request, _ = http.NewRequest(http.MethodPost, "/", nil)

	start := time.Now()
	handler.RevokeConfig(c)
	duration := time.Since(start)

	t.Logf("RevokeConfig took %v for %d AWG keys and %d Xray credentials", duration, numItems, numItems)

	// Expected time is around the time of the slowest single operation (approx 4.5s)
	// If it takes more than 10 seconds, it's definitely running sequentially.
	if duration > 10*time.Second {
		t.Errorf("Performance is sequential! Took %v", duration)
	}
}
