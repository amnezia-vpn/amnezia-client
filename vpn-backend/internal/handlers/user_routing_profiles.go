package handlers

import (
	"log"
	"net/http"
	"strings"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
)

type routingProfileRequest struct {
	Name           string   `json:"name"`
	Action         string   `json:"action"`
	Enabled        *bool    `json:"enabled"`
	Domains        []string `json:"domains"`
	DomainSuffixes []string `json:"domain_suffixes"`
	CIDRs          []string `json:"cidrs"`
}

func normalizeRoutingProfileAction(raw string) models.RoutingProfileAction {
	switch models.RoutingProfileAction(raw) {
	case models.RoutingProfileProxy:
		return models.RoutingProfileProxy
	default:
		return models.RoutingProfileDirect
	}
}

// GET /api/v1/me/routing-profiles
func (h *UserHandler) GetRoutingProfiles(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureRoutingProfileSchema(h.db); err != nil {
		log.Printf("[ROUTING] failed to ensure routing profile schema for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	if err := ensureDefaultRoutingProfiles(h.db, userID); err != nil {
		log.Printf("[ROUTING] failed to seed routing profiles for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	var profiles []models.RoutingProfile
	if err := h.db.Where("user_id = ?", userID).Order("sort_order asc, kind asc, id asc").Find(&profiles).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load routing profiles"})
		return
	}

	customByTemplateCode := buildRoutingTemplateCopyIndex(profiles)
	items := make([]map[string]interface{}, 0, len(profiles))
	for _, profile := range profiles {
		item := routingProfileToMap(profile)
		if profile.Kind == models.RoutingProfileSystem {
			copyProfile, alreadyAdded := customByTemplateCode[profile.Code]
			item["already_added"] = alreadyAdded
			if alreadyAdded {
				item["linked_custom_id"] = copyProfile.ID
			}
		}
		items = append(items, item)
	}

	c.JSON(http.StatusOK, gin.H{"profiles": items})
}

// POST /api/v1/me/routing-profiles
func (h *UserHandler) CreateRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureRoutingProfileSchema(h.db); err != nil {
		log.Printf("[ROUTING] failed to ensure routing profile schema for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}
	if !canManageVIPFeatures(sub) {
		c.JSON(http.StatusForbidden, gin.H{"error": "VIP required"})
		return
	}

	var req routingProfileRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if req.Name == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "name is required"})
		return
	}

	enabled := false
	if req.Enabled != nil {
		enabled = *req.Enabled
	}

	profile := models.RoutingProfile{
		UserID:             userID,
		Name:               req.Name,
		Kind:               models.RoutingProfileCustom,
		Action:             normalizeRoutingProfileAction(req.Action),
		Enabled:            enabled,
		DomainsJSON:        encodeJSONStringArray(req.Domains),
		DomainSuffixesJSON: encodeJSONStringArray(req.DomainSuffixes),
		CIDRsJSON:          encodeJSONStringArray(req.CIDRs),
	}

	if err := h.db.Create(&profile).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to create routing profile"})
		return
	}

	c.JSON(http.StatusCreated, gin.H{"profile": routingProfileToMap(profile)})
}

// PUT /api/v1/me/routing-profiles/:id
func (h *UserHandler) UpdateRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureRoutingProfileSchema(h.db); err != nil {
		log.Printf("[ROUTING] failed to ensure routing profile schema for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}
	if !canManageVIPFeatures(sub) {
		c.JSON(http.StatusForbidden, gin.H{"error": "VIP required"})
		return
	}

	var profile models.RoutingProfile
	if err := h.db.Where("user_id = ?", userID).First(&profile, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "routing profile not found"})
		return
	}

	var req routingProfileRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	updates := map[string]interface{}{}
	if req.Enabled != nil {
		updates["enabled"] = *req.Enabled
	}

	if profile.Kind == models.RoutingProfileSystem {
		if req.Enabled == nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": "nothing to update"})
			return
		}

		if err := h.db.Model(&models.RoutingProfile{}).
			Where("id = ? AND user_id = ?", profile.ID, userID).
			UpdateColumn("enabled", *req.Enabled).Error; err != nil {
			log.Printf("[ROUTING] failed to update system routing profile %d for user %d: %v", profile.ID, userID, err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update routing profile"})
			return
		}
	} else {
		if req.Name != "" {
			updates["name"] = req.Name
		}
		updates["action"] = normalizeRoutingProfileAction(req.Action)
		updates["domains_json"] = encodeJSONStringArray(req.Domains)
		updates["domain_suffixes_json"] = encodeJSONStringArray(req.DomainSuffixes)
		updates["cidrs_json"] = encodeJSONStringArray(req.CIDRs)
		if err := h.db.Model(&profile).Updates(updates).Error; err != nil {
			log.Printf("[ROUTING] failed to update custom routing profile %d for user %d: %v", profile.ID, userID, err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update routing profile"})
			return
		}
	}

	if err := h.db.First(&profile, profile.ID).Error; err != nil {
		log.Printf("[ROUTING] failed to reload routing profile %d for user %d: %v", profile.ID, userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to reload routing profile"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"profile": routingProfileToMap(profile)})
}

// DELETE /api/v1/me/routing-profiles/:id
func (h *UserHandler) DeleteRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureRoutingProfileSchema(h.db); err != nil {
		log.Printf("[ROUTING] failed to ensure routing profile schema for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}
	if !canManageVIPFeatures(sub) {
		c.JSON(http.StatusForbidden, gin.H{"error": "VIP required"})
		return
	}

	var profile models.RoutingProfile
	if err := h.db.Where("user_id = ?", userID).First(&profile, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "routing profile not found"})
		return
	}
	if profile.Kind == models.RoutingProfileSystem {
		c.JSON(http.StatusConflict, gin.H{"error": "system profile cannot be deleted"})
		return
	}

	if err := h.db.Delete(&profile).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to delete routing profile"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "routing profile deleted"})
}

// POST /api/v1/me/routing-profiles/system/:code/copy
func (h *UserHandler) CopySystemRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureRoutingProfileSchema(h.db); err != nil {
		log.Printf("[ROUTING] failed to ensure routing profile schema for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	if err := ensureDefaultRoutingProfiles(h.db, userID); err != nil {
		log.Printf("[ROUTING] failed to seed routing profiles for user %d: %v", userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	sub, err := ensureDefaultSubscription(h.db, userID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load subscription"})
		return
	}
	if !canManageVIPFeatures(sub) {
		c.JSON(http.StatusForbidden, gin.H{"error": "VIP required"})
		return
	}

	code := strings.TrimSpace(c.Param("code"))
	if code == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "system profile code is required"})
		return
	}

	var systemProfile models.RoutingProfile
	if err := h.db.Where("user_id = ? AND kind = ? AND code = ?", userID, models.RoutingProfileSystem, code).
		First(&systemProfile).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "system profile not found"})
		return
	}

	profile, created, err := ensureCustomRoutingProfileFromTemplate(h.db, userID, systemProfile, true)
	if err != nil {
		log.Printf("[ROUTING] failed to copy system profile %q for user %d: %v", code, userID, err)
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to copy system profile"})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"profile": routingProfileToMap(profile),
		"created": created,
	})
}
