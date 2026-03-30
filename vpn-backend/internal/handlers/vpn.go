package handlers

import (
	"crypto/rand"
	"encoding/base64"
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
	"strconv"
	"strings"
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

// GET /api/v1/me/config — получить FBLinkWG2 конфиг совместимый с клиентом
func (h *VPNHandler) GetConfig(c *gin.Context) {
	userID := c.GetUint("user_id")

	// Проверяем активную подписку
	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}
	if sub.Status != models.SubActive {
		c.JSON(http.StatusForbidden, gin.H{"error": "active subscription required"})
		return
	}
	if time.Now().After(sub.ExpiresAt) {
		c.JSON(http.StatusForbidden, gin.H{"error": "subscription expired"})
		return
	}

	capabilities := buildSubscriptionCapabilities(sub)
	if len(capabilities.AllowedProtocols) == 0 {
		c.JSON(http.StatusForbidden, gin.H{"error": "no protocol available for current subscription"})
		return
	}

	// Получаем все активные серверы
	var servers []models.VPNServer
	if err := h.db.Where("active = ?", true).Preload("VLESSTemplate").Find(&servers).Error; err != nil || len(servers) == 0 {
		c.JSON(http.StatusServiceUnavailable, gin.H{"error": "no available VPN servers"})
		return
	}

	responseConfigs := make([]map[string]interface{}, 0, len(servers))
	switch sub.Plan {
	case models.PlanVIP:
		var legacyAWGKeys []models.VPNKey
		h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server").Find(&legacyAWGKeys)
		if len(legacyAWGKeys) > 0 {
			now := time.Now()
			for _, key := range legacyAWGKeys {
				if key.PublicKey != "" {
					if err := removeAWGPeer(&key.Server, key.PublicKey); err != nil {
						fmt.Printf("[WARN] Failed to remove legacy AWG peer for VIP user %d on server %s: %v\n", userID, key.Server.Name, err)
					}
				}
			}
			h.db.Model(&models.VPNKey{}).Where("user_id = ? AND revoked_at IS NULL", userID).Update("revoked_at", &now)
		}

		if err := ensureRoutingProfileSchema(h.db); err != nil {
			log.Printf("[ROUTING] failed to ensure routing profile schema for user %d during config fetch: %v", userID, err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
			return
		}

		if err := ensureDefaultRoutingProfiles(h.db, userID); err != nil {
			log.Printf("[ROUTING] failed to seed routing profiles for user %d during config fetch: %v", userID, err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
			return
		}

		var profiles []models.RoutingProfile
		if err := h.db.Where("user_id = ?", userID).Order("sort_order asc, kind asc, id asc").Find(&profiles).Error; err != nil {
			log.Printf("[ROUTING] failed to load routing profiles for user %d during config fetch: %v", userID, err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load routing profiles"})
			return
		}

		for i := range servers {
			server := &servers[i]
			template, err := ensureVLESSTemplate(h.db, server)
			if err != nil {
				fmt.Printf("[WARN] ensureVLESSTemplate failed for server %s: %v\n", server.Name, err)
				continue
			}
			if template == nil || template.PublicKey == "" || template.ShortID == "" || template.ServerName == "" {
				continue
			}

			clientID := strings.TrimSpace(template.ClientID)
			issuedAt := time.Now()
			if clientID == "" {
				var credential *models.VLESSCredential
				err = h.db.Transaction(func(tx *gorm.DB) error {
					var txErr error
					credential, txErr = ensureVLESSCredential(tx, userID, server, template)
					return txErr
				})
				if err != nil || credential == nil {
					fmt.Printf("[WARN] ensureVLESSCredential failed for server %s: %v\n", server.Name, err)
					continue
				}
				clientID = credential.ClientID
				issuedAt = credential.CreatedAt
			}

			configJSON, err := json.Marshal(buildVLESSConfig(clientID, server, template, profiles))
			if err != nil {
				continue
			}

			responseConfigs = append(responseConfigs, map[string]interface{}{
				"config":    string(configJSON),
				"server":    server.Name,
				"region":    server.Region,
				"issued_at": issuedAt,
			})
		}
	default:
		var legacyVLESS []models.VLESSCredential
		h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server.VLESSTemplate").Find(&legacyVLESS)
		if len(legacyVLESS) > 0 {
			now := time.Now()
			for _, cred := range legacyVLESS {
				if err := removeXrayClient(&cred.Server, cred.Server.VLESSTemplate, cred.ClientID); err != nil {
					fmt.Printf("[WARN] Failed to remove legacy VLESS client for basic user %d on server %s: %v\n", userID, cred.Server.Name, err)
				}
			}
			h.db.Model(&models.VLESSCredential{}).Where("user_id = ? AND revoked_at IS NULL", userID).Update("revoked_at", &now)
		}

		// Получаем все существующие ключи пользователя
		var existingKeys []models.VPNKey
		h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server").Find(&existingKeys)

		keyMap := make(map[uint]*models.VPNKey)
		for i := range existingKeys {
			keyMap[existingKeys[i].ServerID] = &existingKeys[i]
		}

		// Линейный IP-маппинг без коллизий: user_id → 10.8.X.Y
		clientIP := fmt.Sprintf("10.8.%d.%d", (userID-1)/253+1, (userID-1)%253+2)

		for i := range servers {
			server := &servers[i]

			if existingKey, ok := keyMap[server.ID]; ok {
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

			privateKey, publicKey, err := generateWGKeyPair()
			if err != nil {
				continue
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

			if err := addAWGPeer(server, publicKey, presharedKey, clientIP); err != nil {
				fmt.Printf("[WARN] SSH addAWGPeer failed for server %s: %v\n", server.Name, err)
				continue
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
				_ = removeAWGPeer(server, publicKey)
			}
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
		"config":   strings.Join(configLines, "\n"),
		"region":   region,
		"protocol": capabilities.AllowedProtocols[0],
	})
}

// buildAWG2Config формирует JSON-конфиг для FBLink клиента (AWG2 формат)
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

	// Формируем контейнер-обертку, которую ожидает клиент FBLink
	container := map[string]interface{}{
		"container": "fblink-awg",
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
		"defaultContainer": "fblink-awg",
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

	var xrayCredentials []models.VLESSCredential
	h.db.Where("user_id = ? AND revoked_at IS NULL", userID).Preload("Server.VLESSTemplate").Find(&xrayCredentials)
	for _, credential := range xrayCredentials {
		_ = removeXrayClient(&credential.Server, credential.Server.VLESSTemplate, credential.ClientID)
	}

	now := time.Now()
	result := h.db.Model(&models.VPNKey{}).
		Where("user_id = ? AND revoked_at IS NULL", userID).
		Update("revoked_at", now)

	if result.Error != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to revoke key in DB"})
		return
	}

	if err := h.db.Model(&models.VLESSCredential{}).
		Where("user_id = ? AND revoked_at IS NULL", userID).
		Update("revoked_at", now).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to revoke vless credentials in DB"})
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
