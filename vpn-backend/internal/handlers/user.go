package handlers

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"regexp"
	"strings"
	"time"
	"vpn-backend/internal/config"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

type UserHandler struct {
	db  *gorm.DB
	cfg *config.Config
}

var (
	supportBearerPattern = regexp.MustCompile(`(?i)(bearer\s+)[A-Za-z0-9\-_.]+`)
	supportJwtPattern    = regexp.MustCompile(`[A-Za-z0-9\-_]+\.[A-Za-z0-9\-_]+\.[A-Za-z0-9\-_]+`)
	supportAuthPattern   = regexp.MustCompile(`(?i)(authorization["']?\s*:\s*["']?)[^"'\s]+`)
)

func sanitizeSupportText(value string) string {
	sanitized := strings.TrimSpace(value)
	sanitized = supportBearerPattern.ReplaceAllString(sanitized, "${1}***")
	sanitized = supportAuthPattern.ReplaceAllString(sanitized, "${1}***")
	sanitized = supportJwtPattern.ReplaceAllString(sanitized, "***")
	if len(sanitized) > 5000 {
		sanitized = sanitized[:5000]
	}
	return sanitized
}

func NewUserHandler(db *gorm.DB, cfg *config.Config) *UserHandler {
	return &UserHandler{db: db, cfg: cfg}
}

// GET /api/v1/me
func (h *UserHandler) GetMe(c *gin.Context) {
	userID := c.GetUint("user_id")

	var user models.User
	if err := h.db.First(&user, userID).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "user not found"})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"id":         user.ID,
		"email":      user.Email,
		"role":       user.Role,
		"created_at": user.CreatedAt,
	})
}

// GET /api/v1/me/subscription
func (h *UserHandler) GetSubscription(c *gin.Context) {
	userID := c.GetUint("user_id")

	// Пробный период доступен только до первой успешной покупки подписки.
	trialAvailable, err := trialAvailableForUser(h.db, userID)
	if err != nil {
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}

	capabilities := buildSubscriptionCapabilities(sub)
	if capabilities.CanManageRoutingProfiles {
		if err := ensureDefaultRoutingProfiles(h.db, userID); err != nil {
			if isDatabaseBusyError(err) {
				respondDatabaseBusy(c)
				return
			}
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
			return
		}
	}

	c.JSON(http.StatusOK, gin.H{
		"plan":                         sub.Plan,
		"status":                       sub.Status,
		"expires_at":                   sub.ExpiresAt,
		"auto_renew":                   sub.AutoRenew,
		"card_saved":                   sub.PaymentMethodID != "",
		"vip_ad_block_enabled":         sub.VIPAdBlockEnabled,
		"trial_available":              trialAvailable,
		"allowed_protocols":            capabilities.AllowedProtocols,
		"can_use_site_split_tunneling": capabilities.CanUseSiteSplitTunnel,
		"can_use_app_split_tunneling":  capabilities.CanUseAppSplitTunnel,
		"can_manage_routing_profiles":  capabilities.CanManageRoutingProfiles,
		"can_use_ad_block":             capabilities.CanUseAdBlock,
	})
}

// GET /api/v1/me/servers
func (h *UserHandler) GetServers(c *gin.Context) {
	userID := c.GetUint("user_id")

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		if isDatabaseBusyError(err) {
			respondDatabaseBusy(c)
			return
		}
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}

	isVIP := isVIPSubscription(sub)
	var servers []models.VPNServer
	if err := h.db.Where("active = ?", true).Order("id asc").Find(&servers).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load servers"})
		return
	}

	result := make([]gin.H, 0, len(servers))
	for _, server := range servers {
		hostName := strings.TrimSpace(server.Endpoint)
		if hostName == "" {
			hostName = strings.TrimSpace(server.Host)
		}
		result = append(result, gin.H{
			"id":           server.ID,
			"name":         server.Name,
			"region":       server.Region,
			"country_code": server.CountryCode,
			"host_name":    hostName,
			"is_vip_only":  server.VIPOnly,
			"is_available": !server.VIPOnly || isVIP,
		})
	}

	c.JSON(http.StatusOK, gin.H{"servers": result})
}

// PATCH /api/v1/me/subscription/auto-renew
func (h *UserHandler) SetAutoRenew(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req struct {
		Enabled bool `json:"enabled"`
	}
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	var sub models.Subscription
	if err := h.db.Where("user_id = ?", userID).First(&sub).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "subscription not found"})
		return
	}

	h.db.Model(&sub).Update("auto_renew", req.Enabled)
	c.JSON(http.StatusOK, gin.H{"auto_renew": req.Enabled})
}

// PATCH /api/v1/me/subscription/ad-block
func (h *UserHandler) SetVIPAdBlock(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req struct {
		Enabled bool `json:"enabled"`
	}
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}

	if !buildSubscriptionCapabilities(sub).CanUseAdBlock {
		c.JSON(http.StatusForbidden, gin.H{"error": "VIP required"})
		return
	}

	if err := h.db.Model(&models.Subscription{}).
		Where("user_id = ?", userID).
		Update("vip_ad_block_enabled", req.Enabled).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update ad block settings"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"enabled": req.Enabled})
}

// POST /api/v1/me/support/bug-report
func (h *UserHandler) SubmitBugReport(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req struct {
		Note        string                 `json:"note"`
		Diagnostics map[string]interface{} `json:"diagnostics"`
	}
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	ticketID := fmt.Sprintf("FBR-%d-%d", userID, time.Now().Unix())
	sanitizedNote := sanitizeSupportText(req.Note)
	rawDiagnostics, _ := json.Marshal(req.Diagnostics)
	sanitizedDiagnostics := sanitizeSupportText(string(rawDiagnostics))

	log.Printf("[SUPPORT] bug report ticket=%s user_id=%d note=%q diagnostics=%s",
		ticketID, userID, sanitizedNote, sanitizedDiagnostics)

	c.JSON(http.StatusOK, gin.H{
		"ticket_id": ticketID,
		"status":    "accepted",
	})
}

// DELETE /api/v1/me/card — удалить привязанную карту и отключить автосписание
func (h *UserHandler) DeleteCard(c *gin.Context) {
	userID := c.GetUint("user_id")

	var sub models.Subscription
	if err := h.db.Where("user_id = ?", userID).First(&sub).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "subscription not found"})
		return
	}

	h.db.Model(&sub).Updates(map[string]interface{}{
		"payment_method_id": "",
		"auto_renew":        false,
	})
	c.JSON(http.StatusOK, gin.H{"message": "карта удалена, автосписание отключено"})
}
