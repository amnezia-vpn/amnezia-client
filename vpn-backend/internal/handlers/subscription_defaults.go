package handlers

import (
	"time"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

// ensureDefaultSubscription backfills legacy users that were created before
// the default free subscription started being issued at registration time.
func ensureDefaultSubscription(db *gorm.DB, userID uint) (models.Subscription, error) {
	var sub models.Subscription
	err := db.Where("user_id = ?", userID).First(&sub).Error
	if err == nil {
		return sub, nil
	}
	if err != gorm.ErrRecordNotFound {
		return sub, err
	}

	sub = models.Subscription{
		UserID:    userID,
		Plan:      models.PlanFree,
		Status:    models.SubActive,
		ExpiresAt: time.Now().AddDate(1, 0, 0),
	}
	if err := db.Create(&sub).Error; err != nil {
		return sub, err
	}

	return sub, nil
}
