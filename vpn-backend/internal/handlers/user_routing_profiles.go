package handlers

import (
	"net/http"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
)

type routingProfileRequest struct {
	Name           string   `json:"name"`
	Enabled        *bool    `json:"enabled"`
	Domains        []string `json:"domains"`
	DomainSuffixes []string `json:"domain_suffixes"`
	CIDRs          []string `json:"cidrs"`
}

// GET /api/v1/me/routing-profiles
func (h *UserHandler) GetRoutingProfiles(c *gin.Context) {
	userID := c.GetUint("user_id")

	if err := ensureDefaultRoutingProfiles(h.db, userID); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to prepare routing profiles"})
		return
	}

	var profiles []models.RoutingProfile
	if err := h.db.Where("user_id = ?", userID).Order("kind asc, id asc").Find(&profiles).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to load routing profiles"})
		return
	}

	items := make([]map[string]interface{}, 0, len(profiles))
	for _, profile := range profiles {
		items = append(items, routingProfileToMap(profile))
	}

	c.JSON(http.StatusOK, gin.H{"profiles": items})
}

// POST /api/v1/me/routing-profiles
func (h *UserHandler) CreateRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

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
		if len(updates) == 0 {
			c.JSON(http.StatusBadRequest, gin.H{"error": "nothing to update"})
			return
		}
	} else {
		if req.Name != "" {
			updates["name"] = req.Name
		}
		updates["domains_json"] = encodeJSONStringArray(req.Domains)
		updates["domain_suffixes_json"] = encodeJSONStringArray(req.DomainSuffixes)
		updates["cidrs_json"] = encodeJSONStringArray(req.CIDRs)
	}

	if err := h.db.Model(&profile).Updates(updates).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update routing profile"})
		return
	}

	if err := h.db.First(&profile, profile.ID).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to reload routing profile"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"profile": routingProfileToMap(profile)})
}

// DELETE /api/v1/me/routing-profiles/:id
func (h *UserHandler) DeleteRoutingProfile(c *gin.Context) {
	userID := c.GetUint("user_id")

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
