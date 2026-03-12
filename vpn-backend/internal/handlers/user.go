package handlers

import (
	"net/http"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

type UserHandler struct {
	db *gorm.DB
}

func NewUserHandler(db *gorm.DB) *UserHandler {
	return &UserHandler{db: db}
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

	// Проверяем доступность пробного периода
	var trialCount int64
	h.db.Model(&models.Payment{}).
		Where("user_id = ? AND plan = ?", userID, models.PlanTrial).
		Count(&trialCount)
	trialAvailable := trialCount == 0

	var sub models.Subscription
	if err := h.db.Where("user_id = ?", userID).First(&sub).Error; err != nil {
		// Подписки ещё нет — возвращаем дефолтный ответ (не 404)
		c.JSON(http.StatusOK, gin.H{
			"plan":            "none",
			"status":          "none",
			"auto_renew":      false,
			"card_saved":      false,
			"trial_available": trialAvailable,
		})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"plan":            sub.Plan,
		"status":          sub.Status,
		"expires_at":      sub.ExpiresAt,
		"auto_renew":      sub.AutoRenew,
		"card_saved":      sub.PaymentMethodID != "",
		"trial_available": trialAvailable,
	})
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
