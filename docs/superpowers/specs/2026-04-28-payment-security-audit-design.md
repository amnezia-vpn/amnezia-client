# Payment Security Audit — Design Specification

**Date**: 2026-04-28  
**Priority**: High (Financial Risk)  
**Focus**: YooKassa integration security

---

## Problem Statement

Payment system (YooKassa) requires comprehensive security audit:
- **Webhook signature validation** — exists, needs verification
- **Idempotency** — prevent duplicate payments
- **Payment status transitions** — verify state machine correctness
- **Comprehensive audit** — review entire payment flow

**Note**: Trial plan behavior (starts from purchase date) is correct, not a bug.

---

## Current Implementation Review

### Existing Code Location
- **Handlers**: `vpn-backend/internal/handlers/payment.go`
- **Models**: `vpn-backend/internal/models/payment.go`, `subscription.go`
- **Webhook**: `POST /api/v1/payments/webhook`

### Known Behavior
- Trial plan always starts from current date (correct)
- Other plans stack on top of existing `ExpiresAt` if in future
- Webhook signature validation exists (needs verification)

---

## Security Requirements

### 1. Webhook Signature Validation

**Current State**: Implemented (needs audit)

**Requirements**:
```go
// Must verify X-Signature header from YooKassa
// Algorithm: HMAC-SHA256(notification_body, YOOKASSA_SECRET_KEY)

func validateWebhookSignature(body []byte, signature string) bool {
    expectedSignature := hmac.New(sha256.New, []byte(secretKey))
    expectedSignature.Write(body)
    expected := hex.EncodeToString(expectedSignature.Sum(nil))
    
    // Constant-time comparison to prevent timing attacks
    return hmac.Equal([]byte(expected), []byte(signature))
}
```

**Audit Checklist**:
- ✅ Uses constant-time comparison (not `==`)
- ✅ Signature extracted from correct header
- ✅ Secret key from environment variable (not hardcoded)
- ✅ Rejects requests with missing/invalid signature
- ✅ Logs signature validation failures

### 2. Idempotency Protection

**Problem**: Duplicate webhook deliveries can cause double-crediting

**Solution**: Idempotency key tracking

```go
type PaymentIdempotency struct {
    ID              uint      `gorm:"primaryKey"`
    PaymentID       string    `gorm:"uniqueIndex;not null"` // YooKassa payment ID
    WebhookEventID  string    `gorm:"uniqueIndex;not null"` // YooKassa event ID
    ProcessedAt     time.Time `gorm:"not null"`
    Status          string    `gorm:"not null"` // succeeded, failed
}

func processWebhook(paymentID, eventID string) error {
    // Check if already processed
    var existing PaymentIdempotency
    if err := db.Where("payment_id = ? AND webhook_event_id = ?", 
                       paymentID, eventID).First(&existing).Error; err == nil {
        log.Info("Duplicate webhook ignored", "payment_id", paymentID)
        return nil // Already processed
    }
    
    // Process payment in transaction
    tx := db.Begin()
    defer tx.Rollback()
    
    // 1. Record idempotency key
    idem := PaymentIdempotency{
        PaymentID:      paymentID,
        WebhookEventID: eventID,
        ProcessedAt:    time.Now(),
        Status:         "processing",
    }
    if err := tx.Create(&idem).Error; err != nil {
        return err // Duplicate insert will fail (unique constraint)
    }
    
    // 2. Update subscription
    if err := updateSubscription(tx, paymentID); err != nil {
        return err
    }
    
    // 3. Mark as succeeded
    idem.Status = "succeeded"
    tx.Save(&idem)
    
    tx.Commit()
    return nil
}
```

**Audit Checklist**:
- ✅ Idempotency table exists with unique constraints
- ✅ Webhook processing wrapped in transaction
- ✅ Duplicate webhooks return success (not error)
- ✅ Failed processing allows retry (don't mark as succeeded)

### 3. Payment Status State Machine

**Valid Transitions**:
```
pending → succeeded
pending → canceled
pending → waiting_for_capture
waiting_for_capture → succeeded
waiting_for_capture → canceled
```

**Invalid Transitions** (must reject):
```
succeeded → pending (can't un-succeed)
canceled → succeeded (can't revive)
succeeded → canceled (can't cancel after success)
```

**Implementation**:
```go
var validTransitions = map[string][]string{
    "pending":              {"succeeded", "canceled", "waiting_for_capture"},
    "waiting_for_capture":  {"succeeded", "canceled"},
    "succeeded":            {}, // Terminal state
    "canceled":             {}, // Terminal state
}

func validateTransition(from, to string) error {
    allowed, exists := validTransitions[from]
    if !exists {
        return fmt.Errorf("unknown status: %s", from)
    }
    
    for _, valid := range allowed {
        if valid == to {
            return nil
        }
    }
    
    return fmt.Errorf("invalid transition: %s → %s", from, to)
}

func updatePaymentStatus(paymentID, newStatus string) error {
    var payment Payment
    if err := db.First(&payment, "payment_id = ?", paymentID).Error; err != nil {
        return err
    }
    
    // Validate transition
    if err := validateTransition(payment.Status, newStatus); err != nil {
        log.Error("Invalid status transition blocked", 
                  "payment_id", paymentID, 
                  "from", payment.Status, 
                  "to", newStatus)
        return err
    }
    
    payment.Status = newStatus
    return db.Save(&payment).Error
}
```

**Audit Checklist**:
- ✅ State machine enforced in code
- ✅ Invalid transitions rejected and logged
- ✅ Terminal states (succeeded, canceled) cannot change
- ✅ Database constraints prevent invalid states

### 4. Amount and Currency Verification

**Problem**: Webhook could be tampered to show wrong amount

**Solution**: Verify against expected plan price

```go
type Plan struct {
    ID       string
    Name     string
    Price    int64  // In kopecks (rubles * 100)
    Currency string // "RUB"
    Duration int    // Days
}

var plans = map[string]Plan{
    "trial":   {ID: "trial", Price: 0, Currency: "RUB", Duration: 7},
    "monthly": {ID: "monthly", Price: 29900, Currency: "RUB", Duration: 30},
    "yearly":  {ID: "yearly", Price: 299900, Currency: "RUB", Duration: 365},
}

func verifyPaymentAmount(webhook YooKassaWebhook) error {
    plan, exists := plans[webhook.Metadata.PlanID]
    if !exists {
        return fmt.Errorf("unknown plan: %s", webhook.Metadata.PlanID)
    }
    
    // Verify amount
    if webhook.Amount.Value != plan.Price {
        log.Error("Amount mismatch",
                  "expected", plan.Price,
                  "received", webhook.Amount.Value,
                  "payment_id", webhook.ID)
        return fmt.Errorf("amount mismatch")
    }
    
    // Verify currency
    if webhook.Amount.Currency != plan.Currency {
        return fmt.Errorf("currency mismatch")
    }
    
    return nil
}
```

**Audit Checklist**:
- ✅ Amount verified against plan price
- ✅ Currency verified
- ✅ Plan ID validated (exists in system)
- ✅ Mismatches logged and rejected

### 5. Replay Attack Protection

**Problem**: Attacker captures valid webhook, replays it later

**Solution**: Timestamp validation + idempotency

```go
func validateWebhookTimestamp(webhook YooKassaWebhook) error {
    webhookTime, err := time.Parse(time.RFC3339, webhook.CreatedAt)
    if err != nil {
        return fmt.Errorf("invalid timestamp format")
    }
    
    now := time.Now()
    age := now.Sub(webhookTime)
    
    // Reject webhooks older than 5 minutes
    if age > 5*time.Minute {
        log.Warn("Old webhook rejected",
                 "age", age,
                 "payment_id", webhook.ID)
        return fmt.Errorf("webhook too old: %v", age)
    }
    
    // Reject webhooks from future (clock skew tolerance: 1 minute)
    if age < -1*time.Minute {
        return fmt.Errorf("webhook from future")
    }
    
    return nil
}
```

**Audit Checklist**:
- ✅ Timestamp validated
- ✅ Old webhooks rejected (>5 minutes)
- ✅ Future webhooks rejected (clock skew tolerance)
- ✅ Combined with idempotency for full protection

### 6. User Authorization

**Problem**: Webhook must update correct user's subscription

**Solution**: Verify user_id in metadata matches payment

```go
func processPaymentWebhook(webhook YooKassaWebhook) error {
    // Extract user_id from metadata
    userID := webhook.Metadata.UserID
    if userID == 0 {
        return fmt.Errorf("missing user_id in metadata")
    }
    
    // Verify user exists
    var user User
    if err := db.First(&user, userID).Error; err != nil {
        return fmt.Errorf("user not found: %d", userID)
    }
    
    // Verify payment belongs to this user
    var payment Payment
    if err := db.First(&payment, "payment_id = ?", webhook.ID).Error; err == nil {
        if payment.UserID != userID {
            log.Error("User mismatch",
                      "payment_user", payment.UserID,
                      "webhook_user", userID)
            return fmt.Errorf("user mismatch")
        }
    }
    
    // Update subscription
    return updateUserSubscription(userID, webhook.Metadata.PlanID)
}
```

**Audit Checklist**:
- ✅ User ID validated
- ✅ User existence verified
- ✅ Payment-user association verified
- ✅ Prevents crediting wrong user

---

## Comprehensive Audit Checklist

### Input Validation
- [ ] All webhook fields validated (type, format, range)
- [ ] SQL injection prevention (GORM parameterized queries)
- [ ] XSS prevention (no HTML rendering of payment data)
- [ ] Path traversal prevention (no file operations based on webhook data)

### Authentication & Authorization
- [ ] Webhook signature verified (HMAC-SHA256)
- [ ] User authorization checked
- [ ] Admin endpoints require admin role
- [ ] JWT tokens validated on payment status endpoints

### Data Integrity
- [ ] Amount and currency verified
- [ ] Plan ID validated
- [ ] Status transitions enforced
- [ ] Idempotency guaranteed

### Error Handling
- [ ] Errors logged with context (payment_id, user_id)
- [ ] Sensitive data not exposed in errors
- [ ] Failed webhooks return 500 (YooKassa will retry)
- [ ] Invalid webhooks return 400 (no retry)

### Logging & Monitoring
- [ ] All payment events logged
- [ ] Signature validation failures logged
- [ ] Amount mismatches logged
- [ ] Suspicious activity flagged

### Rate Limiting
- [ ] Webhook endpoint rate limited (prevent DoS)
- [ ] Payment creation rate limited per user
- [ ] Admin endpoints rate limited

### Database Security
- [ ] Transactions used for payment processing
- [ ] Unique constraints on idempotency keys
- [ ] Foreign key constraints enforced
- [ ] Sensitive data encrypted at rest (if applicable)

### Secrets Management
- [ ] YOOKASSA_SECRET_KEY from environment
- [ ] YOOKASSA_SHOP_ID from environment
- [ ] No secrets in logs
- [ ] No secrets in error messages

---

## Testing Strategy

### Unit Tests

**Signature Validation** (`payment_test.go`):
```go
func TestWebhookSignatureValidation(t *testing.T) {
    tests := []struct{
        name      string
        body      string
        signature string
        valid     bool
    }{
        {"valid signature", validBody, validSig, true},
        {"invalid signature", validBody, "wrong", false},
        {"missing signature", validBody, "", false},
        {"tampered body", tamperedBody, validSig, false},
    }
    // ...
}
```

**Idempotency** (`payment_test.go`):
```go
func TestIdempotency(t *testing.T) {
    // Process webhook twice
    err1 := processWebhook(paymentID, eventID)
    err2 := processWebhook(paymentID, eventID)
    
    assert.NoError(t, err1)
    assert.NoError(t, err2) // Second call should succeed (idempotent)
    
    // Verify subscription updated only once
    var sub Subscription
    db.First(&sub, userID)
    assert.Equal(t, expectedExpiry, sub.ExpiresAt)
}
```

**State Machine** (`payment_test.go`):
```go
func TestPaymentStatusTransitions(t *testing.T) {
    validTransitions := []struct{from, to string}{
        {"pending", "succeeded"},
        {"pending", "canceled"},
        {"waiting_for_capture", "succeeded"},
    }
    
    invalidTransitions := []struct{from, to string}{
        {"succeeded", "pending"},
        {"canceled", "succeeded"},
        {"succeeded", "canceled"},
    }
    
    // Test valid transitions succeed
    // Test invalid transitions fail
}
```

### Integration Tests

**Full Payment Flow** (`payment_integration_test.go`):
1. User creates payment
2. Simulate YooKassa webhook (succeeded)
3. Verify subscription updated
4. Verify idempotency (replay webhook)
5. Verify amount/currency checked

**Error Scenarios**:
1. Invalid signature → rejected
2. Amount mismatch → rejected
3. Unknown plan → rejected
4. Invalid status transition → rejected
5. Duplicate webhook → idempotent

### Manual Testing

**YooKassa Test Environment**:
1. Create test payment in YooKassa dashboard
2. Trigger webhook manually
3. Verify signature validation
4. Verify subscription updated
5. Test duplicate webhook delivery

**Penetration Testing**:
1. Tamper webhook signature → rejected
2. Modify amount in webhook → rejected
3. Replay old webhook → rejected
4. Send webhook for different user → rejected

---

## Success Criteria

- ✅ Webhook signature validation correct (constant-time comparison)
- ✅ Idempotency prevents duplicate payments
- ✅ Payment status state machine enforced
- ✅ Amount and currency verified
- ✅ Replay attacks prevented
- ✅ User authorization verified
- ✅ All payment events logged
- ✅ No secrets in logs or errors
- ✅ 100% test coverage for payment handlers

---

## Implementation Order

1. **Audit existing signature validation** (verify correctness)
2. **Add idempotency table and logic**
3. **Implement state machine validation**
4. **Add amount/currency verification**
5. **Add timestamp validation (replay protection)**
6. **Add comprehensive logging**
7. **Write unit tests** (signature, idempotency, state machine)
8. **Write integration tests** (full payment flow)
9. **Manual testing** (YooKassa test environment)
10. **Security review** (external audit if possible)

---

## Dependencies

- YooKassa API documentation
- GORM (database ORM)
- Gin (HTTP framework)
- Existing payment models and handlers

---

## Risks & Mitigations

**Risk**: Signature validation has timing attack vulnerability  
**Mitigation**: Use `hmac.Equal()` for constant-time comparison

**Risk**: Race condition in idempotency check  
**Mitigation**: Database unique constraint + transaction

**Risk**: YooKassa changes webhook format  
**Mitigation**: Version webhook handler, log unknown fields

**Risk**: Timezone issues in timestamp validation  
**Mitigation**: Use UTC everywhere, parse RFC3339

---

## Future Enhancements

- Webhook retry queue (if processing fails)
- Payment analytics dashboard
- Fraud detection (unusual payment patterns)
- Refund handling automation
- Subscription renewal reminders
