package handlers

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/http"
	"time"
	"vpn-backend/internal/config"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"gorm.io/gorm"
)

type PaymentHandler struct {
	db             *gorm.DB
	yooKassaShopID string
	yooKassaKey    string
	cfg            *config.Config
}

func NewPaymentHandler(db *gorm.DB, shopID, key string, cfg *config.Config) *PaymentHandler {
	return &PaymentHandler{db: db, yooKassaShopID: shopID, yooKassaKey: key, cfg: cfg}
}

// verifyYooKassaPayment делает GET-запрос к YooKassa API для подтверждения
// реального статуса платежа. Защищает webhook от подделки (forgery).
func (h *PaymentHandler) verifyYooKassaPayment(paymentID string) (string, error) {
	return verifyYooKassaPaymentStatus(h.yooKassaShopID, h.yooKassaKey, paymentID)
}

type createPaymentRequest struct {
	Plan string `json:"plan" binding:"required,oneof=trial basic basic_3m vip vip_3m"`
}

var planPrices = map[models.PlanType]struct {
	Amount       float64
	DurationDays int
}{
	models.PlanTrial:   {Amount: 5.00,    DurationDays: 3},
	models.PlanBasic:   {Amount: 199.00,  DurationDays: 30},
	models.PlanBasic3M: {Amount: 505.00,  DurationDays: 90},
	models.PlanVIP:     {Amount: 399.00,  DurationDays: 30},
	models.PlanVIP3M:   {Amount: 1015.00, DurationDays: 90},
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

	// Пробный период — только если не было успешных trial оплат
	if plan == models.PlanTrial {
		var trialCount int64
		h.db.Model(&models.Payment{}).
			Where("user_id = ? AND plan = ? AND status = ?", userID, models.PlanTrial, models.PaymentSucceeded).
			Count(&trialCount)
		if trialCount > 0 {
			c.JSON(http.StatusConflict, gin.H{"error": "Пробный период уже был использован"})
			return
		}
	}

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
			"return_url": h.cfg.PaymentReturnURL,
		},
		"capture":             true,
		"save_payment_method": true, // сохраняем карту для автосписания
		"description":         fmt.Sprintf("Mr.Frake VPN — %s", planLabel(plan)),
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

	eventType, _ := event["event"].(string)
	if eventType != "payment.succeeded" {
		c.JSON(http.StatusOK, gin.H{"status": "ignored"})
		return
	}

	obj, _ := event["object"].(map[string]interface{})
	ykPaymentID, _ := obj["id"].(string)

	// Верификация через re-fetch к YooKassa API.
	// Защищает от подделки webhook — злоумышленник не может активировать
	// подписку отправив поддельный payload с чужим payment_id.
	actualStatus, err := h.verifyYooKassaPayment(ykPaymentID)
	if err != nil || actualStatus != "succeeded" {
		// Логируем, но отвечаем 200 — иначе YooKassa будет повторно слать webhook
		fmt.Printf("[webhook] Verification failed for payment %s: err=%v status=%s\n", ykPaymentID, err, actualStatus)
		c.JSON(http.StatusOK, gin.H{"status": "verification_failed"})
		return
	}

	var txErr error
	txErr = h.db.Transaction(func(tx *gorm.DB) error {
		var payment models.Payment
		if err := tx.Where("yoo_kassa_id = ?", ykPaymentID).First(&payment).Error; err != nil {
			return err
		}

		if payment.Status == models.PaymentSucceeded {
			return nil // Уже обработано
		}

		now := time.Now()
		tx.Model(&payment).Updates(map[string]interface{}{
			"status":       models.PaymentSucceeded,
			"confirmed_at": now,
		})

		paymentMethodID := ""
		if pm, ok := obj["payment_method"].(map[string]interface{}); ok {
			if saved, _ := pm["saved"].(bool); saved {
				paymentMethodID, _ = pm["id"].(string)
			}
		}

		var sub models.Subscription
		if err := tx.Where("user_id = ?", payment.UserID).First(&sub).Error; err != nil && err != gorm.ErrRecordNotFound {
			return err
		}
		return activateSubscriptionFromPayment(tx, &payment, paymentMethodID)
	})

	if txErr != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "webhook processing failed"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"status": "ok"})
}

func planLabel(plan models.PlanType) string {
	switch plan {
	case models.PlanTrial:
		return "Пробный период (3 дня)"
	case models.PlanBasic:
		return "Базовый (30 дней)"
	case models.PlanBasic3M:
		return "Premium 3 месяца (90 дней)"
	case models.PlanVIP:
		return "VIP (30 дней)"
	case models.PlanVIP3M:
		return "VIP 3 месяца (90 дней)"
	default:
		return string(plan)
	}
}
