package handlers

import (
	"errors"
	"math"
	"net/http"
	"regexp"
	"sort"
	"strings"
	"time"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

var promoCodePattern = regexp.MustCompile(`^[A-Z0-9_-]{3,32}$`)

type promoCodeRequest struct {
	Code            string     `json:"code"`
	Description     string     `json:"description"`
	DiscountPercent int        `json:"discount_percent"`
	MaxUses         int        `json:"max_uses"`
	Active          *bool      `json:"active"`
	ApplicablePlans string     `json:"applicable_plans"`
	OncePerUser     *bool      `json:"once_per_user"`
	ExpiresAt       *time.Time `json:"expires_at"`
}

type promoApplication struct {
	PromoCode      *models.PromoCode
	OriginalAmount float64
	DiscountAmount float64
	FinalAmount    float64
}

func normalizePromoCode(code string) string {
	return strings.ToUpper(strings.TrimSpace(code))
}

func normalizeApplicablePlans(value string) string {
	value = strings.ToLower(strings.TrimSpace(value))
	if value == "" || value == "all" {
		return "all"
	}

	seen := map[string]bool{}
	plans := make([]string, 0)
	for _, item := range strings.Split(value, ",") {
		plan := strings.TrimSpace(item)
		if plan == "" || seen[plan] {
			continue
		}
		if _, ok := planPrices[models.PlanType(plan)]; !ok {
			continue
		}
		seen[plan] = true
		plans = append(plans, plan)
	}
	if len(plans) == 0 {
		return "all"
	}
	sort.Strings(plans)
	return strings.Join(plans, ",")
}

func promoAppliesToPlan(promo models.PromoCode, plan models.PlanType) bool {
	applicable := normalizeApplicablePlans(promo.ApplicablePlans)
	if applicable == "all" {
		return true
	}
	for _, item := range strings.Split(applicable, ",") {
		if strings.TrimSpace(item) == string(plan) {
			return true
		}
	}
	return false
}

func validatePromoCode(db *gorm.DB, userID uint, plan models.PlanType, code string, originalAmount float64) (*promoApplication, error) {
	normalizedCode := normalizePromoCode(code)
	if normalizedCode == "" {
		return &promoApplication{OriginalAmount: originalAmount, FinalAmount: originalAmount}, nil
	}
	if !promoCodePattern.MatchString(normalizedCode) {
		return nil, errors.New("invalid promo code")
	}

	var promo models.PromoCode
	if err := db.Where("code = ?", normalizedCode).First(&promo).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, errors.New("promo code not found")
		}
		return nil, err
	}
	if !promo.Active {
		return nil, errors.New("promo code is disabled")
	}
	if promo.ExpiresAt != nil && promo.ExpiresAt.Before(time.Now()) {
		return nil, errors.New("promo code has expired")
	}
	if promo.MaxUses > 0 && promo.UsedCount >= promo.MaxUses {
		return nil, errors.New("promo code usage limit reached")
	}
	if !promoAppliesToPlan(promo, plan) {
		return nil, errors.New("promo code does not apply to this plan")
	}
	if promo.OncePerUser {
		var usedCount int64
		db.Model(&models.Payment{}).
			Where("user_id = ? AND promo_code_id = ? AND status = ?", userID, promo.ID, models.PaymentSucceeded).
			Count(&usedCount)
		if usedCount > 0 {
			return nil, errors.New("promo code has already been used")
		}
	}

	discountAmount := math.Round(originalAmount*float64(promo.DiscountPercent)) / 100
	if discountAmount > originalAmount {
		discountAmount = originalAmount
	}
	finalAmount := originalAmount - discountAmount
	if finalAmount < 0 {
		finalAmount = 0
	}
	if finalAmount > 0 && finalAmount < 1 {
		finalAmount = 1
		discountAmount = originalAmount - finalAmount
	}

	return &promoApplication{
		PromoCode:      &promo,
		OriginalAmount: originalAmount,
		DiscountAmount: discountAmount,
		FinalAmount:    finalAmount,
	}, nil
}

func markPromoCodeUsed(tx *gorm.DB, payment *models.Payment) error {
	if payment.PromoCodeID == nil {
		return nil
	}
	return tx.Model(&models.PromoCode{}).
		Where("id = ?", *payment.PromoCodeID).
		UpdateColumn("used_count", gorm.Expr("used_count + ?", 1)).
		Error
}

func promoCodeResponse(promo models.PromoCode) gin.H {
	return gin.H{
		"id":               promo.ID,
		"code":             promo.Code,
		"description":      promo.Description,
		"discount_percent": promo.DiscountPercent,
		"max_uses":         promo.MaxUses,
		"used_count":       promo.UsedCount,
		"active":           promo.Active,
		"applicable_plans": promo.ApplicablePlans,
		"once_per_user":    promo.OncePerUser,
		"expires_at":       promo.ExpiresAt,
		"created_at":       promo.CreatedAt,
	}
}

func promoCodeLabel(payment models.Payment) string {
	if payment.PromoCode != nil {
		return payment.PromoCode.Code
	}
	return ""
}

func buildPromoCodeFromRequest(req promoCodeRequest, existing *models.PromoCode) (models.PromoCode, error) {
	active := true
	oncePerUser := true
	if existing != nil {
		active = existing.Active
		oncePerUser = existing.OncePerUser
	}
	if req.Active != nil {
		active = *req.Active
	}
	if req.OncePerUser != nil {
		oncePerUser = *req.OncePerUser
	}

	code := normalizePromoCode(req.Code)
	if existing != nil && code == "" {
		code = existing.Code
	}
	if !promoCodePattern.MatchString(code) {
		return models.PromoCode{}, errors.New("code must be 3-32 characters: A-Z, 0-9, _, -")
	}
	if req.DiscountPercent < 1 || req.DiscountPercent > 100 {
		return models.PromoCode{}, errors.New("discount_percent must be between 1 and 100")
	}
	if req.MaxUses < 0 {
		return models.PromoCode{}, errors.New("max_uses cannot be negative")
	}

	promo := models.PromoCode{
		Code:            code,
		Description:     strings.TrimSpace(req.Description),
		DiscountPercent: req.DiscountPercent,
		MaxUses:         req.MaxUses,
		Active:          active,
		ApplicablePlans: normalizeApplicablePlans(req.ApplicablePlans),
		OncePerUser:     oncePerUser,
		ExpiresAt:       req.ExpiresAt,
	}
	if existing != nil {
		promo.Model = existing.Model
		promo.UsedCount = existing.UsedCount
	}
	return promo, nil
}

func (h *AdminHandler) GetPromoCodes(c *gin.Context) {
	var promoCodes []models.PromoCode
	h.db.Order("created_at desc").Find(&promoCodes)

	result := make([]gin.H, 0, len(promoCodes))
	for _, promo := range promoCodes {
		result = append(result, promoCodeResponse(promo))
	}
	c.JSON(http.StatusOK, gin.H{"promo_codes": result, "total": len(result)})
}

func (h *AdminHandler) CreatePromoCode(c *gin.Context) {
	var req promoCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid payload"})
		return
	}

	promo, err := buildPromoCodeFromRequest(req, nil)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if err := h.db.Create(&promo).Error; err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": "promo code already exists"})
		return
	}
	c.JSON(http.StatusCreated, promoCodeResponse(promo))
}

func (h *AdminHandler) UpdatePromoCode(c *gin.Context) {
	var existing models.PromoCode
	if err := h.db.First(&existing, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "promo code not found"})
		return
	}

	var req promoCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid payload"})
		return
	}

	promo, err := buildPromoCodeFromRequest(req, &existing)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	if err := h.db.Model(&existing).Updates(map[string]interface{}{
		"code":             promo.Code,
		"description":      promo.Description,
		"discount_percent": promo.DiscountPercent,
		"max_uses":         promo.MaxUses,
		"active":           promo.Active,
		"applicable_plans": promo.ApplicablePlans,
		"once_per_user":    promo.OncePerUser,
		"expires_at":       promo.ExpiresAt,
	}).Error; err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": "failed to update promo code"})
		return
	}

	h.db.First(&existing, existing.ID)
	c.JSON(http.StatusOK, promoCodeResponse(existing))
}

func (h *AdminHandler) DeletePromoCode(c *gin.Context) {
	var promo models.PromoCode
	if err := h.db.First(&promo, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "promo code not found"})
		return
	}
	if err := h.db.Model(&promo).Update("active", false).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to disable promo code"})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "promo code disabled"})
}
