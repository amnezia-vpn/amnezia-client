package handlers

import (
	"fmt"
	"time"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

func ensureSubscriptionSchema(db *gorm.DB) error {
	if !db.Migrator().HasColumn(&models.Subscription{}, "VIPAdBlockEnabled") {
		if err := db.Migrator().AddColumn(&models.Subscription{}, "VIPAdBlockEnabled"); err != nil {
			return err
		}
	}

	legacyColumns := []string{
		"v_ip_ad_block_enabled",
		"v_i_p_ad_block_enabled",
	}
	for _, legacyColumn := range legacyColumns {
		if !db.Migrator().HasColumn(&models.Subscription{}, legacyColumn) {
			continue
		}

		if err := db.Exec(fmt.Sprintf(
			`UPDATE subscriptions SET vip_ad_block_enabled = 1 WHERE COALESCE(vip_ad_block_enabled, 0) = 0 AND COALESCE(%s, 0) = 1`,
			legacyColumn,
		)).Error; err != nil {
			return err
		}
	}

	return nil
}

// ensureDefaultSubscription backfills legacy users that were created before
// the default free subscription started being issued at registration time.
func ensureDefaultSubscription(db *gorm.DB, userID uint) (models.Subscription, error) {
	if err := ensureSubscriptionSchema(db); err != nil {
		return models.Subscription{}, err
	}

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
