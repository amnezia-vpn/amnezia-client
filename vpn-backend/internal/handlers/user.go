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

	var sub models.Subscription
	if err := h.db.Where("user_id = ?", userID).First(&sub).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "subscription not found"})
		return
	}

	c.JSON(http.StatusOK, gin.H{
		"plan":       sub.Plan,
		"status":     sub.Status,
		"expires_at": sub.ExpiresAt,
	})
}
