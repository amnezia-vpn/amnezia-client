package handlers

import (
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"net"
	"net/http"
	"strings"
	"strconv"
	"time"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"golang.org/x/crypto/curve25519"
	"gorm.io/gorm"
)

type VPNHandler struct {
	db *gorm.DB
}

func NewVPNHandler(db *gorm.DB) *VPNHandler {
	return &VPNHandler{db: db}
}

// GET /api/v1/me/config — получить AmneziaWG2 конфиг совместимый с клиентом
func (h *VPNHandler) GetConfig(c *gin.Context) {
	userID := c.GetUint("user_id")

	// Проверяем активную подписку
	var sub models.Subscription
	if err := h.db.Where("user_id = ? AND status = ?", userID, models.SubActive).First(&sub).Error; err != nil {
		c.JSON(http.StatusForbidden, gin.H{"error": "active subscription required"})
		return
	}
	if time.Now().After(sub.ExpiresAt) {
		c.JSON(http.StatusForbidden, gin.H{"error": "subscription expired"})
		return
	}

	// Получаем все активные серверы
	var servers []models.VPNServer
	if err := h.db.Where("active = ?", true).Find(&servers).Error; err != nil || len(servers) == 0 {
		c.JSON(http.StatusServiceUnavailable, gin.H{"error": "no available VPN servers"})
		return
	}

	// Получаем все существующие ключи пользователя
	var existingKeys []models.VPNKey
	h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server").Find(&existingKeys)

	keyMap := make(map[uint]*models.VPNKey)
	for i := range existingKeys {
		keyMap[existingKeys[i].ServerID] = &existingKeys[i]
	}

	clientIP := fmt.Sprintf("10.8.%d.%d", (userID/253)%254+1, userID%253+2)

	var responseConfigs []map[string]interface{}

	for i := range servers {
		server := &servers[i]

		// Если для этого сервера уже есть ключ, используем его
		if existingKey, ok := keyMap[server.ID]; ok {
			// Инжектируем актуальный country_code в уже сохранённый конфиг
			configText := existingKey.ConfigText
			configMap := map[string]interface{}{}
			if err := json.Unmarshal([]byte(configText), &configMap); err == nil {
				configMap["country_code"] = existingKey.Server.CountryCode
				if modified, err := json.Marshal(configMap); err == nil {
					configText = string(modified)
				}
			}
			responseConfigs = append(responseConfigs, map[string]interface{}{
				"config":    configText,
				"server":    existingKey.Server.Name,
				"region":    existingKey.Server.Region,
				"issued_at": existingKey.IssuedAt,
			})
			continue
		}

		// Иначе генерируем новый ключ для этого сервера
		privateKey, publicKey, err := generateWGKeyPair()
		if err != nil {
			continue // пропускаем сервер при ошибке генерации
		}
		presharedKey, err := generatePresharedKey()
		if err != nil {
			continue
		}

		endpoint := server.Endpoint
		if endpoint == "" {
			endpoint = fmt.Sprintf("%s:%d", server.Host, server.AWGPort)
		}

		awgConfig := buildAWG2Config(privateKey, publicKey, presharedKey, clientIP, endpoint, server)
		configJSON, err := json.Marshal(awgConfig)
		if err != nil {
			continue
		}

		// Сначала добавляем peer через SSH
		if err := addAWGPeer(server, publicKey, presharedKey, clientIP); err != nil {
			fmt.Printf("[WARN] SSH addAWGPeer failed for server %s: %v\n", server.Name, err)
			continue // Если SSH упал, ключ в БД не пишем!
		}

		now := time.Now()
		key := models.VPNKey{
			UserID:       userID,
			ServerID:     server.ID,
			PublicKey:    publicKey,
			PresharedKey: presharedKey,
			ConfigText:   string(configJSON),
			IssuedAt:     now,
		}

		if err := h.db.Create(&key).Error; err == nil {
			responseConfigs = append(responseConfigs, map[string]interface{}{
				"config":    string(configJSON),
				"server":    server.Name,
				"region":    server.Region,
				"issued_at": now,
			})
		} else {
			// Если БД упала (что редкость), удаляем только что добавленный пир на сервере
			removeAWGPeer(server, publicKey)
		}
	}

	if len(responseConfigs) == 0 {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to generate any configs"})
		return
	}

	// Возвращаем все конфиги через \n — Qt клиент split('\n') парсит каждый отдельно
	var configLines []string
	var region string
	for _, rc := range responseConfigs {
		configLines = append(configLines, rc["config"].(string))
		if region == "" {
			region, _ = rc["region"].(string)
		}
	}
	c.JSON(http.StatusOK, gin.H{
		"config": strings.Join(configLines, "\n"),
		"region": region,
	})
}

// buildAWG2Config формирует JSON-конфиг для AmneziaVPN клиента (AWG2 формат)
func buildAWG2Config(privateKey, clientPublicKey, presharedKey, clientIP, endpoint string, server *models.VPNServer) map[string]interface{} {
	awgConf := fmt.Sprintf(`[Interface]
PrivateKey = %s
Address = %s/24
DNS = 1.1.1.1, 8.8.8.8
Jc = %s
Jmin = %s
Jmax = %s
S1 = %s
S2 = %s
S3 = %s
S4 = %s
H1 = %s
H2 = %s
H3 = %s
H4 = %s

[Peer]
PublicKey = %s
PresharedKey = %s
Endpoint = %s
AllowedIPs = 0.0.0.0/0, ::/0
PersistentKeepalive = 25
`,
		privateKey, clientIP,
		server.Jc, server.Jmin, server.Jmax,
		server.S1, server.S2, server.S3, server.S4,
		server.H1, server.H2, server.H3, server.H4,
		server.PublicKey, presharedKey, endpoint,
	)

	endpointIP := server.Host
	endpointPort := strconv.Itoa(server.AWGPort)
	if server.Endpoint != "" {
		if host, port, err := net.SplitHostPort(server.Endpoint); err == nil {
			endpointIP = host
			endpointPort = port
		} else {
			endpointIP = server.Endpoint
		}
	}
	portInt, _ := strconv.Atoi(endpointPort)

	// Формируем конфиг протокола AWG (используем короткие ключи как в protocols_defs.h)
	protocolConfig := map[string]interface{}{
		"config_version":    "3",
		"config":            awgConf,
		"dns1":              "1.1.1.1",
		"dns2":              "8.8.8.8",
		"Jc":                server.Jc,
		"Jmin":              server.Jmin,
		"Jmax":              server.Jmax,
		"S1":                server.S1,
		"S2":                server.S2,
		"S3":                server.S3,
		"S4":                server.S4,
		"H1":                server.H1,
		"H2":                server.H2,
		"H3":                server.H3,
		"H4":                server.H4,
		"I1":                server.I1,
		"I2":                server.I2,
		"I3":                server.I3,
		"I4":                server.I4,
		"I5":                server.I5,
		"mtu":               server.MTU,
		"deviceMTU":         server.MTU,
		"last_config":       time.Now().Unix(),
		"hostName":          endpointIP,
		"serverIpv4AddrIn":  endpointIP,
		"serverIpv4Gateway": endpointIP,
		"port":              portInt,
		"serverPort":        float64(portInt),
		"privateKey":        privateKey,
		"client_priv_key":   privateKey,
		"deviceIpv4Address": clientIP,
		"client_ip":         clientIP,
		"serverPublicKey":   server.PublicKey,
		"server_pub_key":    server.PublicKey,
		"psk_key":           presharedKey,
		"serverPskKey":      presharedKey,
	}

	protocolConfigJSON, _ := json.Marshal(protocolConfig)

	// Формируем контейнер-обертку, которую ожидает клиент AmneziaVPN
	container := map[string]interface{}{
		"container": "amnezia-awg",
		"awg": map[string]interface{}{
			"last_config":        string(protocolConfigJSON),
			"isThirdPartyConfig": true,
			"port":               endpointPort,
			"transport_proto":    "udp",
			"protocolVersion":    "1.5",
		},
	}

	// Итоговый JSON конфиг
	return map[string]interface{}{
		"containers":       []interface{}{container},
		"defaultContainer": "amnezia-awg",
		"description":      "FBLink VPN - " + server.Region,
		"country_code":     server.CountryCode,
		"dns1":             "1.1.1.1",
		"dns2":             "8.8.8.8",
		"hostName":         server.Host,
	}
}

// POST /api/v1/me/config/revoke — отозвать текущий ключ
func (h *VPNHandler) RevokeConfig(c *gin.Context) {
	userID := c.GetUint("user_id")

	var keysToRevoke []models.VPNKey
	h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server").Find(&keysToRevoke)

	if len(keysToRevoke) == 0 {
		c.JSON(http.StatusNotFound, gin.H{"error": "no active key found"})
		return
	}

	for _, k := range keysToRevoke {
		if k.PublicKey != "" {
			if err := removeAWGPeer(&k.Server, k.PublicKey); err != nil {
				fmt.Printf("[WARN] SSH removeAWGPeer failed for server %s: %v\n", k.Server.Name, err)
			}
		}
	}

	now := time.Now()
	result := h.db.Model(&models.VPNKey{}).
		Where("user_id = ? AND revoked_at IS NULL", userID).
		Update("revoked_at", now)

	if result.Error != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to revoke key in DB"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "key revoked successfully"})
}

// generateWGKeyPair генерирует WireGuard/AWG приватный и публичный ключи (Curve25519)
func generateWGKeyPair() (privateKeyB64, publicKeyB64 string, err error) {
	var privateKey [32]byte
	if _, err = rand.Read(privateKey[:]); err != nil {
		return
	}

	// WireGuard clamp bits (RFC 7748)
	privateKey[0] &= 248
	privateKey[31] &= 127
	privateKey[31] |= 64

	publicKey, e := curve25519.X25519(privateKey[:], curve25519.Basepoint)
	if e != nil {
		err = e
		return
	}

	privateKeyB64 = base64.StdEncoding.EncodeToString(privateKey[:])
	publicKeyB64 = base64.StdEncoding.EncodeToString(publicKey)
	return
}

// generatePresharedKey генерирует WireGuard PresharedKey (32 случайных байта base64)
func generatePresharedKey() (string, error) {
	var psk [32]byte
	if _, err := rand.Read(psk[:]); err != nil {
		return "", err
	}
	return base64.StdEncoding.EncodeToString(psk[:]), nil
}
