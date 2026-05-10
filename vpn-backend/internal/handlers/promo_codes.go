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

func promoError(message string) error {
	return errors.New(message)
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
		return nil, promoError("Промокод должен содержать 3-32 символа: латинские буквы, цифры, _ или -")
	}

	var promo models.PromoCode
	if err := db.Where("code = ?", normalizedCode).First(&promo).Error; err != nil {
		if errors.Is(err, gorm.ErrRecordNotFound) {
			return nil, promoError("Промокод не найден")
		}
		return nil, err
	}
	if !promo.Active {
		return nil, promoError("Промокод отключён")
	}
	if promo.ExpiresAt != nil && promo.ExpiresAt.Before(time.Now()) {
		return nil, promoError("Срок действия промокода истёк")
	}
	actualUsedCount := promoSuccessfulUseCount(db, promo.ID)
	if promo.MaxUses > 0 && actualUsedCount >= int64(promo.MaxUses) {
		return nil, promoError("Лимит использований промокода исчерпан")
	}
	if !promoAppliesToPlan(promo, plan) {
		return nil, promoError("Промокод не действует на выбранный тариф")
	}
	if promo.OncePerUser {
		var usedCount int64
		db.Model(&models.Payment{}).
			Where("user_id = ? AND promo_code_id = ? AND status = ?", userID, promo.ID, models.PaymentSucceeded).
			Count(&usedCount)
		if usedCount > 0 {
			return nil, promoError("Вы уже использовали этот промокод")
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

func promoSuccessfulUseCount(db *gorm.DB, promoCodeID uint) int64 {
	var count int64
	db.Model(&models.Payment{}).
		Where("promo_code_id = ? AND status = ?", promoCodeID, models.PaymentSucceeded).
		Count(&count)
	return count
}

func markPromoCodeUsed(tx *gorm.DB, payment *models.Payment) error {
	if payment.PromoCodeID == nil {
		return nil
	}
	usedCount := promoSuccessfulUseCount(tx, *payment.PromoCodeID)
	return tx.Model(&models.PromoCode{}).
		Where("id = ?", *payment.PromoCodeID).
		UpdateColumn("used_count", usedCount).
		Error
}

func promoCodeResponse(db *gorm.DB, promo models.PromoCode) gin.H {
	usedCount := promoSuccessfulUseCount(db, promo.ID)
	return gin.H{
		"id":               promo.ID,
		"code":             promo.Code,
		"description":      promo.Description,
		"discount_percent": promo.DiscountPercent,
		"max_uses":         promo.MaxUses,
		"used_count":       usedCount,
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
		return models.PromoCode{}, errors.New("Код должен содержать 3-32 символа: A-Z, 0-9, _ или -")
	}
	if req.DiscountPercent < 1 || req.DiscountPercent > 100 {
		return models.PromoCode{}, errors.New("Скидка должна быть от 1 до 100%")
	}
	if req.MaxUses < 0 {
		return models.PromoCode{}, errors.New("Лимит использований не может быть отрицательным")
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
		result = append(result, promoCodeResponse(h.db, promo))
	}
	c.JSON(http.StatusOK, gin.H{"promo_codes": result, "total": len(result)})
}

func (h *AdminHandler) CreatePromoCode(c *gin.Context) {
	var req promoCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Некорректные данные промокода"})
		return
	}

	promo, err := buildPromoCodeFromRequest(req, nil)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	if err := h.db.Create(&promo).Error; err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": "Промокод с таким кодом уже существует"})
		return
	}
	c.JSON(http.StatusCreated, promoCodeResponse(h.db, promo))
}

func (h *AdminHandler) UpdatePromoCode(c *gin.Context) {
	var existing models.PromoCode
	if err := h.db.First(&existing, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "Промокод не найден"})
		return
	}

	var req promoCodeRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "Некорректные данные промокода"})
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
		c.JSON(http.StatusConflict, gin.H{"error": "Не удалось сохранить промокод"})
		return
	}

	h.db.First(&existing, existing.ID)
	c.JSON(http.StatusOK, promoCodeResponse(h.db, existing))
}

func (h *AdminHandler) DeletePromoCode(c *gin.Context) {
	var promo models.PromoCode
	if err := h.db.First(&promo, c.Param("id")).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "Промокод не найден"})
		return
	}
	if err := h.db.Model(&promo).Update("active", false).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "Не удалось отключить промокод"})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "Промокод отключён"})
}
