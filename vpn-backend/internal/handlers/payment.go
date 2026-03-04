package handlers

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"time"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"
)

type PaymentHandler struct {
	db             *gorm.DB
	yooKassaShopID string
	yooKassaKey    string
}

func NewPaymentHandler(db *gorm.DB, shopID, key string) *PaymentHandler {
	return &PaymentHandler{db: db, yooKassaShopID: shopID, yooKassaKey: key}
}

type createPaymentRequest struct {
	Plan string `json:"plan" binding:"required,oneof=basic premium"`
}

var planPrices = map[models.PlanType]struct {
	Amount       float64
	DurationDays int
}{
	models.PlanBasic:   {Amount: 199.00, DurationDays: 30},
	models.PlanPremium: {Amount: 499.00, DurationDays: 30},
}

// POST /api/v1/payments/create
func (h *PaymentHandler) CreatePayment(c *gin.Context) {
	userID := c.GetUint("user_id")

	var req createPaymentRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	plan := models.PlanType(req.Plan)
	priceInfo := planPrices[plan]

	// Создаём платёж в ЮKassa
	idempotencyKey := uuid.New().String()
	ykPayload := map[string]interface{}{
		"amount": map[string]interface{}{
			"value":    fmt.Sprintf("%.2f", priceInfo.Amount),
			"currency": "RUB",
		},
		"confirmation": map[string]interface{}{
			"type":       "redirect",
			"return_url": "https://yourvpn.app/payment/success",
		},
		"capture":     true,
		"description": fmt.Sprintf("VPN план: %s", plan),
		"metadata": map[string]interface{}{
			"user_id": userID,
			"plan":    plan,
		},
	}

	body, _ := json.Marshal(ykPayload)
	httpReq, _ := http.NewRequest("POST", "https://api.yookassa.ru/v3/payments", bytes.NewReader(body))
	httpReq.Header.Set("Content-Type", "application/json")
	httpReq.Header.Set("Idempotence-Key", idempotencyKey)
	httpReq.SetBasicAuth(h.yooKassaShopID, h.yooKassaKey)

	resp, err := http.DefaultClient.Do(httpReq)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to create payment"})
		return
	}
	defer resp.Body.Close()

	var ykResp map[string]interface{}
	json.NewDecoder(resp.Body).Decode(&ykResp)

	if resp.StatusCode != http.StatusOK && resp.StatusCode != http.StatusCreated {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "YooKassa error", "details": ykResp})
		return
	}

	confirmURL := ""
	if conf, ok := ykResp["confirmation"].(map[string]interface{}); ok {
		confirmURL, _ = conf["confirmation_url"].(string)
	}

	// Сохраняем платёж в БД
	payment := models.Payment{
		UserID:     userID,
		YooKassaID: fmt.Sprintf("%v", ykResp["id"]),
		Amount:     priceInfo.Amount,
		Currency:   "RUB",
		Status:     models.PaymentPending,
		Plan:       plan,
		ConfirmURL: confirmURL,
	}
	h.db.Create(&payment)

	c.JSON(http.StatusOK, gin.H{
		"payment_id":       payment.ID,
		"confirmation_url": confirmURL,
		"amount":           priceInfo.Amount,
		"plan":             plan,
	})
}

// POST /api/v1/payments/webhook — вебхук от ЮKassa
func (h *PaymentHandler) Webhook(c *gin.Context) {
	var event map[string]interface{}
	if err := c.ShouldBindJSON(&event); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid payload"})
		return
	}

	eventType, _ := event["type"].(string)
	if eventType != "payment.succeeded" {
		c.JSON(http.StatusOK, gin.H{"status": "ignored"})
		return
	}

	obj, _ := event["object"].(map[string]interface{})
	ykPaymentID, _ := obj["id"].(string)

	// Находим платёж
	var payment models.Payment
	if err := h.db.Where("yoo_kassa_id = ?", ykPaymentID).First(&payment).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "payment not found"})
		return
	}

	if payment.Status == models.PaymentSucceeded {
		c.JSON(http.StatusOK, gin.H{"status": "already processed"})
		return
	}

	// Обновляем статус платежа
	now := time.Now()
	h.db.Model(&payment).Updates(map[string]interface{}{
		"status":       models.PaymentSucceeded,
		"confirmed_at": now,
	})

	// Обновляем или продлеваем подписку
	priceInfo := planPrices[payment.Plan]
	newExpiry := now.AddDate(0, 0, priceInfo.DurationDays)

	var sub models.Subscription
	if err := h.db.Where("user_id = ?", payment.UserID).First(&sub).Error; err != nil {
		// Создаём новую
		sub = models.Subscription{
			UserID:    payment.UserID,
			Plan:      payment.Plan,
			Status:    models.SubActive,
			ExpiresAt: newExpiry,
		}
		h.db.Create(&sub)
	} else {
		// Продлеваем
		if sub.ExpiresAt.After(now) {
			newExpiry = sub.ExpiresAt.AddDate(0, 0, priceInfo.DurationDays)
		}
		h.db.Model(&sub).Updates(map[string]interface{}{
			"plan":       payment.Plan,
			"status":     models.SubActive,
			"expires_at": newExpiry,
		})
	}

	c.JSON(http.StatusOK, gin.H{"status": "ok"})
}
