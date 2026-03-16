package handlers

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"time"
	"vpn-backend/internal/models"

	"github.com/google/uuid"
	"gorm.io/gorm"
)

// RunAutoRenewalScheduler проверяет каждый час подписки, истекшие сегодня,
// и списывает оплату с сохранённой карты. Вызывать как горутину из main.
func RunAutoRenewalScheduler(db *gorm.DB, shopID, key string) {
	if shopID == "" || key == "" {
		log.Println("[renewal] YooKassa не настроена — автосписание отключено")
		return
	}

	log.Println("[renewal] Планировщик автосписания запущен (интервал: 1ч)")

	ticker := time.NewTicker(1 * time.Hour)
	defer ticker.Stop()

	for range ticker.C {
		processAutoRenewals(db, shopID, key)
	}
}

func processAutoRenewals(db *gorm.DB, shopID, key string) {
	now := time.Now()

	// 1. Автосписание для подписок с картой и включённым auto_renew
	var subs []models.Subscription
	db.Where(
		"auto_renew = true AND payment_method_id != '' AND status = ? AND expires_at <= ?",
		models.SubActive, now,
	).Find(&subs)

	if len(subs) > 0 {
		log.Printf("[renewal] Найдено %d истёкших подписок к автосписанию", len(subs))
		for _, sub := range subs {
			if err := chargeAutoRenewal(db, shopID, key, sub); err != nil {
				log.Printf("[renewal] Ошибка списания user_id=%d: %v", sub.UserID, err)
			}
		}
	}

	// 2. Пометить как expired все оставшиеся просроченные активные подписки
	//    (без карты, с отключённым auto_renew, или у кого списание не прошло)
	result := db.Model(&models.Subscription{}).
		Where("status = ? AND expires_at <= ?", models.SubActive, now).
		Update("status", models.SubExpired)
	if result.RowsAffected > 0 {
		log.Printf("[renewal] Помечено как expired: %d подписок", result.RowsAffected)
	}
}

func chargeAutoRenewal(db *gorm.DB, shopID, key string, sub models.Subscription) error {
	priceInfo, ok := planPrices[sub.Plan]
	if !ok {
		return fmt.Errorf("неизвестный план: %s", sub.Plan)
	}

	idempotencyKey := uuid.New().String()
	payload := map[string]interface{}{
		"amount": map[string]interface{}{
			"value":    fmt.Sprintf("%.2f", priceInfo.Amount),
			"currency": "RUB",
		},
		"capture":           true,
		"payment_method_id": sub.PaymentMethodID,
		"description":       fmt.Sprintf("Автопродление VPN %s (user_id=%d)", sub.Plan, sub.UserID),
		"metadata": map[string]interface{}{
			"user_id":    sub.UserID,
			"plan":       sub.Plan,
			"auto_renew": true,
		},
	}

	body, _ := json.Marshal(payload)
	req, _ := http.NewRequest("POST", "https://api.yookassa.ru/v3/payments", bytes.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Idempotence-Key", idempotencyKey)
	req.SetBasicAuth(shopID, key)

	resp, err := http.DefaultClient.Do(req)
	if err != nil {
		return fmt.Errorf("запрос к YooKassa: %w", err)
	}
	defer resp.Body.Close()

	var ykResp map[string]interface{}
	json.NewDecoder(resp.Body).Decode(&ykResp)

	if resp.StatusCode != http.StatusOK && resp.StatusCode != http.StatusCreated {
		return fmt.Errorf("YooKassa вернула %d: %v", resp.StatusCode, ykResp)
	}

	status, _ := ykResp["status"].(string)
	ykPaymentID, _ := ykResp["id"].(string)

	// Сохраняем платёж в БД
	now := time.Now()
	payment := models.Payment{
		UserID:     sub.UserID,
		YooKassaID: ykPaymentID,
		Amount:     priceInfo.Amount,
		Currency:   "RUB",
		Plan:       sub.Plan,
	}

	if status == "succeeded" {
		payment.Status = models.PaymentSucceeded
		payment.ConfirmedAt = &now

		// Продлеваем подписку сразу
		newExpiry := sub.ExpiresAt.AddDate(0, 0, priceInfo.DurationDays)
		db.Model(&sub).Updates(map[string]interface{}{
			"status":     models.SubActive,
			"expires_at": newExpiry,
		})
		log.Printf("[renewal] Подписка user_id=%d продлена до %s", sub.UserID, newExpiry.Format("2006-01-02"))
	} else {
		// pending — вебхук payment.succeeded сам продлит подписку
		payment.Status = models.PaymentPending
		log.Printf("[renewal] Платёж user_id=%d создан, ожидаем вебхук (status=%s)", sub.UserID, status)
	}

	db.Create(&payment)
	return nil
}
