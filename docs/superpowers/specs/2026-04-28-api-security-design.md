# API Security — Design Specification

**Date**: 2026-04-28  
**Priority**: High (Foundation Security)  
**Focus**: Input validation + secrets management + comprehensive audit

---

## Problem Statement

API security requires comprehensive improvements:
- **Input validation** — all endpoints need robust validation
- **Secrets management** — multiple issues (A-F from brainstorming)
- **Comprehensive audit** — review entire API surface

---

## Scope

### API Endpoints to Audit

**Public Endpoints** (no auth):
- `POST /api/v1/auth/register`
- `POST /api/v1/auth/login`
- `POST /api/v1/auth/refresh`
- `POST /api/v1/payments/webhook` (YooKassa signature auth)

**User Endpoints** (JWT required):
- `GET /api/v1/me/profile`
- `PUT /api/v1/me/profile`
- `GET /api/v1/me/subscription`
- `GET /api/v1/me/vpn/config` (WireGuard config generation)
- `POST /api/v1/me/vpn/connect`
- `POST /api/v1/me/vpn/disconnect`

**Payment Endpoints** (JWT required):
- `POST /api/v1/payments/create`
- `GET /api/v1/payments/:id`
- `GET /api/v1/payments/history`

**Admin Endpoints** (JWT + admin role):
- `GET /api/v1/admin/users`
- `PUT /api/v1/admin/users/:id`
- `DELETE /api/v1/admin/users/:id`
- `GET /api/v1/admin/servers`
- `POST /api/v1/admin/servers`
- `GET /api/v1/admin/stats`

---

## Input Validation Framework

### Validation Library

Use **go-playground/validator** for declarative validation:

```go
import "github.com/go-playground/validator/v10"

var validate = validator.New()

type RegisterRequest struct {
    Email    string `json:"email" validate:"required,email,max=255"`
    Password string `json:"password" validate:"required,min=8,max=128"`
    Name     string `json:"name" validate:"required,min=2,max=100"`
}

func (h *AuthHandler) Register(c *gin.Context) {
    var req RegisterRequest
    if err := c.ShouldBindJSON(&req); err != nil {
        c.JSON(400, gin.H{"error": "Invalid JSON"})
        return
    }
    
    // Validate
    if err := validate.Struct(req); err != nil {
        c.JSON(400, gin.H{"error": formatValidationErrors(err)})
        return
    }
    
    // Sanitize
    req.Email = strings.ToLower(strings.TrimSpace(req.Email))
    req.Name = strings.TrimSpace(req.Name)
    
    // Process...
}
```

### Validation Rules by Field Type

**Email**:
```go
Email string `validate:"required,email,max=255"`
// Additional: check disposable email domains (optional)
```

**Password**:
```go
Password string `validate:"required,min=8,max=128,password_strength"`
// Custom validator: at least 1 uppercase, 1 lowercase, 1 digit
```

**Phone** (if used):
```go
Phone string `validate:"omitempty,e164"` // E.164 format: +79991234567
```

**User ID** (path/query params):
```go
UserID uint `validate:"required,min=1"`
```

**Plan ID**:
```go
PlanID string `validate:"required,oneof=trial monthly yearly"`
```

**Server ID**:
```go
ServerID string `validate:"required,uuid4"`
```

**Pagination**:
```go
Page  int `validate:"min=1,max=1000"`
Limit int `validate:"min=1,max=100"`
```

**Sort**:
```go
Sort string `validate:"omitempty,oneof=asc desc"`
```

### Custom Validators

**Password Strength**:
```go
func validatePasswordStrength(fl validator.FieldLevel) bool {
    password := fl.Field().String()
    
    hasUpper := regexp.MustCompile(`[A-Z]`).MatchString(password)
    hasLower := regexp.MustCompile(`[a-z]`).MatchString(password)
    hasDigit := regexp.MustCompile(`[0-9]`).MatchString(password)
    
    return hasUpper && hasLower && hasDigit
}

// Register custom validator
validate.RegisterValidation("password_strength", validatePasswordStrength)
```

**No SQL Keywords** (defense in depth):
```go
func validateNoSQLKeywords(fl validator.FieldLevel) bool {
    value := strings.ToLower(fl.Field().String())
    sqlKeywords := []string{"select", "insert", "update", "delete", "drop", "union", "--", "/*"}
    
    for _, keyword := range sqlKeywords {
        if strings.Contains(value, keyword) {
            return false
        }
    }
    return true
}
```

---

## Secrets Management

### Current Issues (A-F)

**A. .env files not in .gitignore**
- Risk: Secrets committed to git
- Solution: Add to .gitignore, scan git history

**B. Hardcoded secrets in code**
- Risk: Secrets in source code
- Solution: Audit codebase, move to environment variables

**C. Secrets in logs or error messages**
- Risk: Secrets exposed in logs
- Solution: Sanitize logs, never log sensitive fields

**D. No secret rotation process**
- Risk: Compromised secrets remain valid forever
- Solution: Document rotation procedure, implement rotation support

**E. Secrets transmitted insecurely**
- Risk: Plaintext transmission
- Solution: HTTPS only, verify TLS configuration

**F. No secrets management documentation**
- Risk: Inconsistent handling, deployment issues
- Solution: Create comprehensive documentation

### Solution Architecture

#### 1. Environment Variables (Current)

**Structure** (`vpn-backend/.env`):
```bash
# Database
DB_PATH=/var/lib/fblinkvpn/database.db

# JWT
JWT_SECRET=<random-256-bit-hex>
JWT_EXPIRY=24h
JWT_REFRESH_EXPIRY=168h

# YooKassa
YOOKASSA_SHOP_ID=<shop-id>
YOOKASSA_SECRET_KEY=<secret-key>

# SMTP
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_USER=<email>
SMTP_PASSWORD=<app-password>
SMTP_FROM=noreply@fblinkvpn.com

# S3 (if used)
S3_ENDPOINT=<endpoint>
S3_ACCESS_KEY=<key>
S3_SECRET_KEY=<secret>
S3_BUCKET=<bucket>

# Server
PORT=8080
GIN_MODE=release
```

**Loading** (`internal/config/config.go`):
```go
type Config struct {
    DBPath            string `env:"DB_PATH,required"`
    JWTSecret         string `env:"JWT_SECRET,required"`
    JWTExpiry         string `env:"JWT_EXPIRY" envDefault:"24h"`
    YooKassaShopID    string `env:"YOOKASSA_SHOP_ID,required"`
    YooKassaSecretKey string `env:"YOOKASSA_SECRET_KEY,required"`
    SMTPHost          string `env:"SMTP_HOST,required"`
    SMTPPort          int    `env:"SMTP_PORT,required"`
    SMTPUser          string `env:"SMTP_USER,required"`
    SMTPPassword      string `env:"SMTP_PASSWORD,required"`
    // ...
}

func Load() (*Config, error) {
    cfg := &Config{}
    if err := env.Parse(cfg); err != nil {
        return nil, fmt.Errorf("failed to parse config: %w", err)
    }
    
    // Validate required secrets are not empty
    if err := validateSecrets(cfg); err != nil {
        return nil, err
    }
    
    return cfg, nil
}
```

#### 2. .gitignore Protection

**Add to `.gitignore`**:
```gitignore
# Environment files
.env
.env.*
!.env.example

# Backup files
*.db
*.db-journal
*.db-wal

# Logs
*.log
logs/

# Secrets
secrets/
*.key
*.pem
*.crt
```

**Create `.env.example`** (template without secrets):
```bash
# Database
DB_PATH=/var/lib/fblinkvpn/database.db

# JWT
JWT_SECRET=<generate-with-openssl-rand-hex-32>
JWT_EXPIRY=24h

# YooKassa
YOOKASSA_SHOP_ID=<your-shop-id>
YOOKASSA_SECRET_KEY=<your-secret-key>

# SMTP
SMTP_HOST=smtp.gmail.com
SMTP_PORT=587
SMTP_USER=<your-email>
SMTP_PASSWORD=<your-app-password>
SMTP_FROM=noreply@fblinkvpn.com
```

#### 3. Git History Scan

**Scan for committed secrets**:
```bash
# Install git-secrets
git clone https://github.com/awslabs/git-secrets.git
cd git-secrets && make install

# Scan repository
cd /path/to/fblinkvpn
git secrets --scan-history

# Or use truffleHog
docker run --rm -v $(pwd):/repo trufflesecurity/trufflehog:latest \
    filesystem /repo --json
```

**If secrets found**:
1. Rotate compromised secrets immediately
2. Use BFG Repo-Cleaner to remove from history
3. Force push (coordinate with team)

#### 4. Log Sanitization

**Sanitize sensitive fields**:
```go
type SanitizedLogger struct {
    logger *log.Logger
}

var sensitiveFields = []string{
    "password", "token", "secret", "key", "authorization",
    "jwt", "smtp_password", "yookassa_secret",
}

func (l *SanitizedLogger) Info(msg string, fields ...interface{}) {
    sanitized := sanitizeFields(fields)
    l.logger.Info(msg, sanitized...)
}

func sanitizeFields(fields []interface{}) []interface{} {
    result := make([]interface{}, len(fields))
    for i := 0; i < len(fields); i += 2 {
        key := fields[i].(string)
        value := fields[i+1]
        
        if isSensitive(key) {
            result[i] = key
            result[i+1] = "***REDACTED***"
        } else {
            result[i] = key
            result[i+1] = value
        }
    }
    return result
}

func isSensitive(key string) bool {
    keyLower := strings.ToLower(key)
    for _, sensitive := range sensitiveFields {
        if strings.Contains(keyLower, sensitive) {
            return true
        }
    }
    return false
}
```

**Error messages**:
```go
// BAD: Exposes secret
return fmt.Errorf("JWT validation failed with secret: %s", secret)

// GOOD: No secret exposure
return fmt.Errorf("JWT validation failed")
```

#### 5. Secret Rotation Process

**Documentation** (`docs/secrets-rotation.md`):
```markdown
# Secret Rotation Procedure

## JWT Secret Rotation

1. Generate new secret:
   ```bash
   openssl rand -hex 32
   ```

2. Update `.env` with new secret (keep old one temporarily)

3. Deploy new version (supports both old and new secrets during transition)

4. Wait for all active tokens to expire (24h for access, 7d for refresh)

5. Remove old secret from `.env`

6. Deploy final version

## YooKassa Secret Rotation

1. Generate new secret in YooKassa dashboard
2. Update `.env` with new secret
3. Test webhook with new secret
4. Deploy
5. Revoke old secret in YooKassa dashboard

## SMTP Password Rotation

1. Generate new app password in email provider
2. Update `.env`
3. Test email sending
4. Deploy
5. Revoke old app password
```

**Implementation** (dual-secret support during rotation):
```go
type Config struct {
    JWTSecret    string `env:"JWT_SECRET,required"`
    JWTSecretOld string `env:"JWT_SECRET_OLD"` // Optional, for rotation
}

func validateJWT(tokenString string, cfg *Config) (*Claims, error) {
    // Try new secret first
    claims, err := parseToken(tokenString, cfg.JWTSecret)
    if err == nil {
        return claims, nil
    }
    
    // If old secret configured, try it
    if cfg.JWTSecretOld != "" {
        claims, err = parseToken(tokenString, cfg.JWTSecretOld)
        if err == nil {
            log.Warn("Token validated with old secret, rotation in progress")
            return claims, nil
        }
    }
    
    return nil, fmt.Errorf("invalid token")
}
```

#### 6. HTTPS Enforcement

**Gin middleware**:
```go
func HTTPSRedirect() gin.HandlerFunc {
    return func(c *gin.Context) {
        if c.Request.Header.Get("X-Forwarded-Proto") != "https" {
            httpsURL := "https://" + c.Request.Host + c.Request.RequestURI
            c.Redirect(301, httpsURL)
            c.Abort()
            return
        }
        c.Next()
    }
}

// Apply to all routes
router.Use(HTTPSRedirect())
```

**TLS Configuration**:
```go
tlsConfig := &tls.Config{
    MinVersion:               tls.VersionTLS12,
    CurvePreferences:         []tls.CurveID{tls.CurveP521, tls.CurveP384, tls.CurveP256},
    PreferServerCipherSuites: true,
    CipherSuites: []uint16{
        tls.TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
        tls.TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
        tls.TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305,
    },
}

server := &http.Server{
    Addr:      ":443",
    Handler:   router,
    TLSConfig: tlsConfig,
}

server.ListenAndServeTLS("cert.pem", "key.pem")
```

---

## Comprehensive API Audit

### Security Checklist per Endpoint

For EACH endpoint, verify:

#### Authentication & Authorization
- [ ] Correct auth middleware applied (public/user/admin)
- [ ] JWT validation correct (signature, expiration, claims)
- [ ] User ID from JWT matches resource owner
- [ ] Admin role checked for admin endpoints
- [ ] Rate limiting applied

#### Input Validation
- [ ] All inputs validated (body, query, path params)
- [ ] Type validation (string, int, email, UUID, etc.)
- [ ] Length validation (min, max)
- [ ] Format validation (email, phone, date, etc.)
- [ ] Enum validation (allowed values)
- [ ] Sanitization applied (trim, lowercase, etc.)

#### SQL Injection Prevention
- [ ] GORM parameterized queries used (no raw SQL)
- [ ] No string concatenation in queries
- [ ] User input never directly in SQL

#### Output Sanitization
- [ ] No sensitive data in responses (passwords, secrets)
- [ ] Error messages don't leak internal details
- [ ] Stack traces disabled in production

#### Business Logic
- [ ] Authorization checks (user can access resource)
- [ ] State validation (valid transitions)
- [ ] Idempotency (where applicable)
- [ ] Race condition prevention (transactions)

#### Logging
- [ ] Request logged (method, path, user_id)
- [ ] Errors logged with context
- [ ] Sensitive fields redacted
- [ ] No secrets in logs

### Example: Register Endpoint Audit

**Endpoint**: `POST /api/v1/auth/register`

**Current Implementation** (assumed):
```go
func (h *AuthHandler) Register(c *gin.Context) {
    var req RegisterRequest
    c.BindJSON(&req)
    
    // Create user
    user := User{Email: req.Email, Password: req.Password}
    db.Create(&user)
    
    c.JSON(200, user)
}
```

**Issues**:
1. ❌ No input validation
2. ❌ Password stored in plaintext
3. ❌ No duplicate email check
4. ❌ No error handling
5. ❌ Returns password in response

**Secure Implementation**:
```go
type RegisterRequest struct {
    Email    string `json:"email" validate:"required,email,max=255"`
    Password string `json:"password" validate:"required,min=8,max=128,password_strength"`
    Name     string `json:"name" validate:"required,min=2,max=100"`
}

func (h *AuthHandler) Register(c *gin.Context) {
    var req RegisterRequest
    
    // Parse JSON
    if err := c.ShouldBindJSON(&req); err != nil {
        log.Error("Invalid JSON", "error", err)
        c.JSON(400, gin.H{"error": "Invalid request format"})
        return
    }
    
    // Validate
    if err := validate.Struct(req); err != nil {
        log.Error("Validation failed", "error", err)
        c.JSON(400, gin.H{"error": formatValidationErrors(err)})
        return
    }
    
    // Sanitize
    req.Email = strings.ToLower(strings.TrimSpace(req.Email))
    req.Name = strings.TrimSpace(req.Name)
    
    // Check duplicate email
    var existing User
    if err := h.db.Where("email = ?", req.Email).First(&existing).Error; err == nil {
        log.Warn("Duplicate email registration attempt", "email", req.Email)
        c.JSON(409, gin.H{"error": "Email already registered"})
        return
    }
    
    // Hash password
    hashedPassword, err := bcrypt.GenerateFromPassword([]byte(req.Password), bcrypt.DefaultCost)
    if err != nil {
        log.Error("Password hashing failed", "error", err)
        c.JSON(500, gin.H{"error": "Internal server error"})
        return
    }
    
    // Create user
    user := User{
        Email:    req.Email,
        Password: string(hashedPassword),
        Name:     req.Name,
        Role:     "user",
    }
    
    if err := h.db.Create(&user).Error; err != nil {
        log.Error("User creation failed", "error", err)
        c.JSON(500, gin.H{"error": "Failed to create user"})
        return
    }
    
    log.Info("User registered", "user_id", user.ID, "email", user.Email)
    
    // Return safe response (no password)
    c.JSON(201, gin.H{
        "id":    user.ID,
        "email": user.Email,
        "name":  user.Name,
    })
}
```

**Checklist**:
- ✅ Input validation (email, password strength, name)
- ✅ Sanitization (trim, lowercase)
- ✅ Duplicate check
- ✅ Password hashing (bcrypt)
- ✅ Error handling
- ✅ Logging (no sensitive data)
- ✅ Safe response (no password)

---

## Testing Strategy

### Unit Tests

**Input Validation** (`handlers_test.go`):
```go
func TestRegisterValidation(t *testing.T) {
    tests := []struct{
        name    string
        request RegisterRequest
        wantErr bool
    }{
        {"valid", RegisterRequest{Email: "test@example.com", Password: "Pass123!", Name: "Test"}, false},
        {"invalid email", RegisterRequest{Email: "invalid", Password: "Pass123!", Name: "Test"}, true},
        {"weak password", RegisterRequest{Email: "test@example.com", Password: "weak", Name: "Test"}, true},
        {"empty name", RegisterRequest{Email: "test@example.com", Password: "Pass123!", Name: ""}, true},
    }
    
    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            err := validate.Struct(tt.request)
            if (err != nil) != tt.wantErr {
                t.Errorf("validation error = %v, wantErr %v", err, tt.wantErr)
            }
        })
    }
}
```

**Secrets Sanitization** (`logger_test.go`):
```go
func TestLogSanitization(t *testing.T) {
    fields := []interface{}{
        "email", "test@example.com",
        "password", "secret123",
        "jwt_token", "eyJhbGc...",
    }
    
    sanitized := sanitizeFields(fields)
    
    assert.Equal(t, "test@example.com", sanitized[1]) // Email not sanitized
    assert.Equal(t, "***REDACTED***", sanitized[3])   // Password sanitized
    assert.Equal(t, "***REDACTED***", sanitized[5])   // Token sanitized
}
```

### Integration Tests

**Full API Flow** (`api_test.go`):
1. Register user → verify validation
2. Login → verify JWT issued
3. Access protected endpoint → verify auth
4. Access admin endpoint as user → verify 403
5. Invalid JWT → verify 401

**SQL Injection Attempts** (`security_test.go`):
```go
func TestSQLInjectionPrevention(t *testing.T) {
    maliciousInputs := []string{
        "'; DROP TABLE users; --",
        "1' OR '1'='1",
        "admin'--",
    }
    
    for _, input := range maliciousInputs {
        resp := registerUser(RegisterRequest{
            Email:    input,
            Password: "Pass123!",
            Name:     "Test",
        })
        
        // Should fail validation, not execute SQL
        assert.Equal(t, 400, resp.StatusCode)
    }
}
```

### Security Audit Tools

**Static Analysis**:
```bash
# gosec - security scanner
go install github.com/securego/gosec/v2/cmd/gosec@latest
gosec ./...

# golangci-lint with security checks
golangci-lint run --enable=gosec,gas
```

**Dependency Scanning**:
```bash
# nancy - dependency vulnerability scanner
go list -json -m all | nancy sleuth
```

**Secret Scanning**:
```bash
# git-secrets
git secrets --scan

# truffleHog
trufflehog filesystem . --json
```

---

## Success Criteria

### Input Validation
- ✅ All endpoints have validation
- ✅ Custom validators for password strength, etc.
- ✅ Sanitization applied consistently
- ✅ 100% test coverage for validation logic

### Secrets Management
- ✅ .env in .gitignore
- ✅ No hardcoded secrets in code
- ✅ Secrets never in logs or errors
- ✅ Rotation process documented and tested
- ✅ HTTPS enforced
- ✅ Deployment documentation complete

### Comprehensive Audit
- ✅ All endpoints audited (checklist completed)
- ✅ SQL injection prevention verified
- ✅ Authentication/authorization correct
- ✅ Rate limiting applied
- ✅ Security tools pass (gosec, nancy)

---

## Implementation Order

1. **Add validation library** (go-playground/validator)
2. **Implement custom validators** (password strength, etc.)
3. **Audit and fix auth endpoints** (register, login, refresh)
4. **Audit and fix user endpoints** (profile, subscription, VPN config)
5. **Audit and fix payment endpoints** (create, webhook, history)
6. **Audit and fix admin endpoints** (users, servers, stats)
7. **Implement log sanitization**
8. **Fix .gitignore and scan git history**
9. **Document secret rotation process**
10. **Enforce HTTPS**
11. **Write comprehensive tests**
12. **Run security audit tools**
13. **External security review** (if possible)

---

## Dependencies

- go-playground/validator (validation)
- bcrypt (password hashing)
- GORM (ORM, SQL injection prevention)
- Gin (HTTP framework)

---

## Risks & Mitigations

**Risk**: Validation too strict, breaks existing clients  
**Mitigation**: Version API, deprecate old endpoints gradually

**Risk**: Secret rotation causes downtime  
**Mitigation**: Dual-secret support during transition

**Risk**: Log sanitization misses new sensitive fields  
**Mitigation**: Whitelist approach (log only known-safe fields)

**Risk**: Git history contains secrets  
**Mitigation**: Rotate all secrets before cleaning history

---

## Future Enhancements

- API versioning (v2 with stricter validation)
- GraphQL API (with built-in validation)
- Secrets management service (HashiCorp Vault)
- Automated security scanning in CI/CD
- Bug bounty program
