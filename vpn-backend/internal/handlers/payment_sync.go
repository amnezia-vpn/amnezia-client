package handlers

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

const yooKassaStatusTimeout = 1500 * time.Millisecond

var yooKassaStatusHTTPClient = &http.Client{
	Timeout: yooKassaStatusTimeout,
}

func verifyYooKassaPaymentStatus(shopID, key, paymentID string) (string, error) {
	req, err := http.NewRequest("GET", "https://api.yookassa.ru/v3/payments/"+paymentID, nil)
	if err != nil {
		return "", err
	}
	req.SetBasicAuth(shopID, key)

	resp, err := yooKassaStatusHTTPClient.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	if resp.StatusCode < http.StatusOK || resp.StatusCode >= http.StatusMultipleChoices {
		return "", fmt.Errorf("yookassa status check returned http %d", resp.StatusCode)
	}

	body, _ := io.ReadAll(resp.Body)
	var data map[string]interface{}
	if err := json.Unmarshal(body, &data); err != nil {
		return "", err
	}

	status, _ := data["status"].(string)
	return status, nil
}

func activateSubscriptionFromPayment(tx *gorm.DB, payment *models.Payment, paymentMethodID string) error {
	now := time.Now()
	priceInfo := planPrices[payment.Plan]
	autoRenew := payment.Plan != models.PlanTrial && paymentMethodID != ""

	var sub models.Subscription
	if err := tx.Where("user_id = ?", payment.UserID).First(&sub).Error; err != nil {
		sub = models.Subscription{
			UserID:          payment.UserID,
			Plan:            payment.Plan,
			Status:          models.SubActive,
			ExpiresAt:       now.AddDate(0, 0, priceInfo.DurationDays),
			AutoRenew:       autoRenew,
			PaymentMethodID: paymentMethodID,
		}
		return tx.Create(&sub).Error
	}

	var newExpiry time.Time
	if payment.Plan != models.PlanTrial && sub.ExpiresAt.After(now) && sub.Plan != models.PlanFree {
		newExpiry = sub.ExpiresAt.AddDate(0, 0, priceInfo.DurationDays)
	} else {
		newExpiry = now.AddDate(0, 0, priceInfo.DurationDays)
	}

	updates := map[string]interface{}{
		"plan":       payment.Plan,
		"status":     models.SubActive,
		"expires_at": newExpiry,
		"auto_renew": autoRenew,
	}
	if paymentMethodID != "" {
		updates["payment_method_id"] = paymentMethodID
	}

	return tx.Model(&sub).Updates(updates).Error
}
