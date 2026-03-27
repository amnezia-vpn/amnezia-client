package handlers

import (
	"bytes"
	"encoding/csv"
	"fmt"
	"net/http"
	"sort"
	"time"
	"vpn-backend/internal/backup"
	"vpn-backend/internal/config"
	"vpn-backend/internal/models"

	"github.com/gin-gonic/gin"
	"gorm.io/gorm"
)

type AdminHandler struct {
	db  *gorm.DB
	cfg *config.Config
}

func NewAdminHandler(db *gorm.DB, cfg *config.Config) *AdminHandler {
	return &AdminHandler{db: db, cfg: cfg}
}

// POST /api/v1/admin/backup/send — ручной запуск бэкапа
func (h *AdminHandler) TriggerBackup(c *gin.Context) {
	if h.cfg.SMTPHost == "" {
		c.JSON(http.StatusServiceUnavailable, gin.H{"error": "SMTP не настроен (SMTP_HOST пустой)"})
		return
	}
	if err := backup.DoBackup(h.db, h.cfg); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusOK, gin.H{"message": "Бэкап успешно отправлен на email всех администраторов"})
}

// GET /api/v1/admin/users
func (h *AdminHandler) GetUsers(c *gin.Context) {
	var users []models.User
	h.db.Preload("Subscription").Find(&users)

	result := make([]gin.H, 0, len(users))
	for _, u := range users {
		sub := gin.H{"plan": "none"}
		if u.Subscription != nil {
			sub = gin.H{
				"plan":       u.Subscription.Plan,
				"status":     u.Subscription.Status,
				"expires_at": u.Subscription.ExpiresAt,
			}
		}
		result = append(result, gin.H{
			"id":           u.ID,
			"email":        u.Email,
			"role":         u.Role,
			"created_at":   u.CreatedAt,
			"subscription": sub,
		})
	}

	c.JSON(http.StatusOK, gin.H{"users": result, "total": len(result)})
}

// GET /api/v1/admin/servers
func (h *AdminHandler) GetServers(c *gin.Context) {
	var servers []models.VPNServer
	h.db.Find(&servers)

	result := make([]gin.H, 0, len(servers))
	for _, s := range servers {
		var peersCount int64
		h.db.Model(&models.VPNKey{}).Where("server_id = ? AND revoked_at IS NULL", s.ID).Count(&peersCount)
		result = append(result, gin.H{
			"id":           s.ID,
			"name":         s.Name,
			"host":         s.Host,
			"endpoint":     s.Endpoint,
			"region":       s.Region,
			"country_code": s.CountryCode,
			"active":       s.Active,
			"max_peers":    s.MaxPeers,
			"active_keys":  peersCount,
			"awg_port":     s.AWGPort,
		})
	}

	c.JSON(http.StatusOK, gin.H{"servers": result})
}

type addServerRequest struct {
	Name        string `json:"name" binding:"required"`
	Host        string `json:"host" binding:"required"`
	Endpoint    string `json:"endpoint"` // IP:port для клиентов
	PublicKey   string `json:"public_key"`
	Region      string `json:"region"`
	CountryCode string `json:"country_code"` // ISO 3166-1 alpha-2, e.g. "RU"
	MaxPeers    int    `json:"max_peers"`
	AWGPort     int    `json:"awg_port"`
	MTU         string `json:"mtu"`
	// SSH доступ для управления peers
	SSHHost      string `json:"ssh_host"`
	SSHPort      int    `json:"ssh_port"`
	SSHUser      string `json:"ssh_user"`
	SSHPassword  string `json:"ssh_password"`
	RootPassword string `json:"root_password"`
	AWGContainer string `json:"awg_container"`
	AWGInterface string `json:"awg_interface"`
	// Обфускация AWG2
	Jc   string `json:"jc"`
	Jmin string `json:"jmin"`
	Jmax string `json:"jmax"`
	S1   string `json:"s1"`
	S2   string `json:"s2"`
	S3   string `json:"s3"` // AWG2: cookie reply
	S4   string `json:"s4"` // AWG2: transport
	H1   string `json:"h1"`
	H2   string `json:"h2"`
	H3   string `json:"h3"`
	H4   string `json:"h4"`
	I1   string `json:"i1"`
	I2   string `json:"i2"`
	I3   string `json:"i3"`
	I4   string `json:"i4"`
	I5   string `json:"i5"`
}

// POST /api/v1/admin/servers
func (h *AdminHandler) AddServer(c *gin.Context) {
	var req addServerRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	maxPeers := req.MaxPeers
	if maxPeers <= 0 {
		maxPeers = 100
	}
	awgPort := req.AWGPort
	if awgPort <= 0 {
		awgPort = 51820
	}
	mtu := req.MTU
	if mtu == "" {
		mtu = "1280"
	}
	jc := req.Jc
	if jc == "" {
		jc = "4"
	}
	jmin := req.Jmin
	if jmin == "" {
		jmin = "40"
	}
	jmax := req.Jmax
	if jmax == "" {
		jmax = "70"
	}
	sshPort := req.SSHPort
	if sshPort == 0 {
		sshPort = 22
	}
	sshPassword := req.SSHPassword
	if sshPassword == "" {
		sshPassword = req.RootPassword
	}
	awgContainer := req.AWGContainer
	if awgContainer == "" {
		awgContainer = "amnezia-awg2"
	}
	awgIface := req.AWGInterface
	if awgIface == "" {
		awgIface = "awg0"
	}
	h1 := req.H1
	if h1 == "" {
		h1 = "1"
	}
	h2 := req.H2
	if h2 == "" {
		h2 = "2"
	}
	h3 := req.H3
	if h3 == "" {
		h3 = "3"
	}
	h4 := req.H4
	if h4 == "" {
		h4 = "4"
	}

	server := models.VPNServer{
		Name:         req.Name,
		Host:         req.Host,
		Endpoint:     req.Endpoint,
		PublicKey:    req.PublicKey,
		Region:       req.Region,
		CountryCode:  req.CountryCode,
		MaxPeers:     maxPeers,
		Active:       true,
		AWGPort:      awgPort,
		MTU:          mtu,
		Jc:           jc,
		Jmin:         jmin,
		Jmax:         jmax,
		S1:           req.S1,
		S2:           req.S2,
		S3:           req.S3,
		S4:           req.S4,
		H1:           h1,
		H2:           h2,
		H3:           h3,
		H4:           h4,
		I1:           req.I1,
		I2:           req.I2,
		I3:           req.I3,
		I4:           req.I4,
		I5:           req.I5,
		SSHHost:      req.SSHHost,
		SSHPort:      sshPort,
		SSHUser:      req.SSHUser,
		SSHPassword:  sshPassword,
		AWGContainer: awgContainer,
		AWGInterface: awgIface,
	}

	if server.PublicKey == "" {
		pubKey, err := fetchServerPublicKey(&server)
		if err != nil {
			fmt.Printf("[ERROR] fetchServerPublicKey failed: %v\n", err)
			c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to fetch public key from server via SSH: " + err.Error()})
			return
		}
		server.PublicKey = pubKey

		// Также попытаемся вытащить параметры обфускации из awg0.conf
		if err := fetchServerConfigHeaders(&server); err != nil {
			fmt.Printf("[WARN] Failed to fetch obfuscation headers: %v\n", err)
		}
	}

	if err := h.db.Create(&server).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to add server"})
		return
	}

	c.JSON(http.StatusCreated, server)
}

// PUT /api/v1/admin/servers/:id
func (h *AdminHandler) UpdateServer(c *gin.Context) {
	id := c.Param("id")
	var s models.VPNServer
	if err := h.db.First(&s, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "server not found"})
		return
	}

	var req struct {
		Name        string `json:"name"`
		Region      string `json:"region"`
		CountryCode string `json:"country_code"`
		Endpoint    string `json:"endpoint"`
		MaxPeers    int    `json:"max_peers"`
		SSHPassword string `json:"ssh_password"`
		AWGPort     int    `json:"awg_port"`
	}
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	updates := map[string]interface{}{}
	if req.Name != "" {
		updates["name"] = req.Name
	}
	if req.Region != "" {
		updates["region"] = req.Region
	}
	updates["country_code"] = req.CountryCode // разрешаем очищать
	if req.Endpoint != "" {
		updates["endpoint"] = req.Endpoint
	}
	if req.MaxPeers > 0 {
		updates["max_peers"] = req.MaxPeers
	}
	if req.SSHPassword != "" {
		updates["ssh_password"] = req.SSHPassword
	}
	if req.AWGPort > 0 {
		updates["awg_port"] = req.AWGPort
	}

	if err := h.db.Model(&s).Updates(updates).Error; err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to update server"})
		return
	}

	c.JSON(http.StatusOK, gin.H{"message": "server updated"})
}

// DELETE /api/v1/admin/servers/:id
func (h *AdminHandler) DeleteServer(c *gin.Context) {
	id := c.Param("id")

	var s models.VPNServer
	if err := h.db.First(&s, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "server not found"})
		return
	}

	// 1. Находим все ключи, привязанные к этому серверу
	var keys []models.VPNKey
	h.db.Where("server_id = ?", id).Find(&keys)

	// 2. Удаляем пиров с самого сервера через SSH (если они не отозваны)
	for _, k := range keys {
		if k.RevokedAt == nil && k.PublicKey != "" {
			if err := removeAWGPeer(&s, k.PublicKey); err != nil {
				fmt.Printf("[WARN] Failed to remove peer %s during server deletion: %v\n", k.PublicKey, err)
			}
		}
	}

	// 3. Удаляем ключи из БД
	h.db.Where("server_id = ?", id).Delete(&models.VPNKey{})

	// 4. Удаляем сам сервер
	h.db.Delete(&s)

	c.JSON(http.StatusOK, gin.H{"message": "server and all associated keys deleted permanently"})
}

// DELETE /api/v1/admin/users/:id
func (h *AdminHandler) DeleteUser(c *gin.Context) {
	id := c.Param("id")

	var u models.User
	if err := h.db.First(&u, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "user not found"})
		return
	}

	// 1. Находим все ключи пользователя для удаления с серверов
	var keys []models.VPNKey
	h.db.Where("user_id = ?", id).Preload("Server").Find(&keys)

	for _, k := range keys {
		if k.RevokedAt == nil && k.PublicKey != "" {
			if err := removeAWGPeer(&k.Server, k.PublicKey); err != nil {
				fmt.Printf("[WARN] Failed to remove peer %s during user deletion: %v\n", k.PublicKey, err)
			}
		}
	}

	// 2. Удаляем все связанные данные из БД
	h.db.Unscoped().Where("user_id = ?", id).Delete(&models.VPNKey{})
	h.db.Unscoped().Where("user_id = ?", id).Delete(&models.Subscription{})
	h.db.Unscoped().Where("user_id = ?", id).Delete(&models.Payment{})

	// 3. Удаляем пользователя
	h.db.Unscoped().Delete(&u)

	c.JSON(http.StatusOK, gin.H{"message": "user and all associated data deleted permanently"})
}

// GET /api/v1/admin/payments
func (h *AdminHandler) GetPayments(c *gin.Context) {
	var payments []models.Payment
	h.db.Preload("User").Order("created_at desc").Limit(100).Find(&payments)

	result := make([]gin.H, 0, len(payments))
	for _, p := range payments {
		result = append(result, gin.H{
			"id":           p.ID,
			"user_email":   p.User.Email,
			"amount":       p.Amount,
			"plan":         p.Plan,
			"status":       p.Status,
			"created_at":   p.CreatedAt,
			"confirmed_at": p.ConfirmedAt,
		})
	}

	c.JSON(http.StatusOK, gin.H{"payments": result, "total": len(result)})
}

// GET /api/v1/admin/stats
func (h *AdminHandler) GetStats(c *gin.Context) {
	var totalUsers, activeSubscriptions, expiredSubscriptions, cancelledSubscriptions, activeKeys int64
	var totalServers, activeServers int64
	var totalRevenue, monthlyRevenue float64
	var pendingPayments int64
	windowDays := 7
	if c.Query("days") == "30" {
		windowDays = 30
	}

	h.db.Model(&models.User{}).Count(&totalUsers)
	h.db.Model(&models.Subscription{}).Where("status = ?", models.SubActive).Count(&activeSubscriptions)
	h.db.Model(&models.Subscription{}).Where("status = ?", models.SubExpired).Count(&expiredSubscriptions)
	h.db.Model(&models.Subscription{}).Where("status = ?", models.SubCancelled).Count(&cancelledSubscriptions)
	h.db.Model(&models.VPNKey{}).Where("revoked_at IS NULL").Count(&activeKeys)
	h.db.Model(&models.VPNServer{}).Count(&totalServers)
	h.db.Model(&models.VPNServer{}).Where("active = ?", true).Count(&activeServers)
	h.db.Model(&models.Payment{}).Where("status = ?", models.PaymentPending).Count(&pendingPayments)

	now := time.Now()
	startOfMonth := time.Date(now.Year(), now.Month(), 1, 0, 0, 0, 0, now.Location())

	h.db.Model(&models.Payment{}).
		Where("status = ?", models.PaymentSucceeded).
		Select("COALESCE(SUM(amount), 0)").
		Scan(&totalRevenue)

	h.db.Model(&models.Payment{}).
		Where("status = ? AND confirmed_at >= ?", models.PaymentSucceeded, startOfMonth).
		Select("COALESCE(SUM(amount), 0)").
		Scan(&monthlyRevenue)

	startWindow := time.Date(now.Year(), now.Month(), now.Day(), 0, 0, 0, 0, now.Location()).AddDate(0, 0, -(windowDays - 1))

	var recentUsers []models.User
	h.db.Where("created_at >= ?", startWindow).Find(&recentUsers)

	var recentPayments []models.Payment
	h.db.Where("created_at >= ?", startWindow).Find(&recentPayments)

	userDays := map[string]int{}
	revenueDays := map[string]float64{}
	for i := 0; i < windowDays; i++ {
		dayKey := startWindow.AddDate(0, 0, i).Format("2006-01-02")
		userDays[dayKey] = 0
		revenueDays[dayKey] = 0
	}

	for _, u := range recentUsers {
		dayKey := u.CreatedAt.Format("2006-01-02")
		userDays[dayKey]++
	}

	for _, p := range recentPayments {
		if p.Status != models.PaymentSucceeded {
			continue
		}
		dayKey := p.CreatedAt.Format("2006-01-02")
		revenueDays[dayKey] += p.Amount
	}

	usersSeries := make([]gin.H, 0, windowDays)
	revenueSeries := make([]gin.H, 0, windowDays)
	for i := 0; i < windowDays; i++ {
		day := startWindow.AddDate(0, 0, i)
		dayKey := day.Format("2006-01-02")
		usersSeries = append(usersSeries, gin.H{
			"date":  dayKey,
			"label": day.Format("02.01"),
			"value": userDays[dayKey],
		})
		revenueSeries = append(revenueSeries, gin.H{
			"date":  dayKey,
			"label": day.Format("02.01"),
			"value": revenueDays[dayKey],
		})
	}

	var subscriptions []models.Subscription
	h.db.Find(&subscriptions)
	planBreakdown := map[string]int{
		string(models.PlanFree):  0,
		string(models.PlanTrial): 0,
		string(models.PlanBasic): 0,
	}
	statusBreakdown := map[string]int{
		string(models.SubActive):    0,
		string(models.SubExpired):   0,
		string(models.SubCancelled): 0,
	}
	autoRenewEnabled := 0
	for _, sub := range subscriptions {
		planBreakdown[string(sub.Plan)]++
		statusBreakdown[string(sub.Status)]++
		if sub.AutoRenew {
			autoRenewEnabled++
		}
	}

	var servers []models.VPNServer
	h.db.Find(&servers)
	serverLoad := make([]gin.H, 0, len(servers))
	regionTotals := map[string]int{}
	for _, s := range servers {
		var peersCount int64
		h.db.Model(&models.VPNKey{}).Where("server_id = ? AND revoked_at IS NULL", s.ID).Count(&peersCount)

		utilization := 0.0
		if s.MaxPeers > 0 {
			utilization = (float64(peersCount) / float64(s.MaxPeers)) * 100
		}

		regionKey := s.Region
		if regionKey == "" {
			regionKey = s.Name
		}
		regionTotals[regionKey] += int(peersCount)

		serverLoad = append(serverLoad, gin.H{
			"id":           s.ID,
			"name":         s.Name,
			"region":       s.Region,
			"country_code": s.CountryCode,
			"active":       s.Active,
			"active_keys":  peersCount,
			"max_peers":    s.MaxPeers,
			"utilization":  utilization,
			"available":    maxInt(s.MaxPeers-int(peersCount), 0),
			"endpoint":     s.Endpoint,
		})
	}

	sort.Slice(serverLoad, func(i, j int) bool {
		return serverLoad[i]["utilization"].(float64) > serverLoad[j]["utilization"].(float64)
	})

	topRegions := make([]gin.H, 0, len(regionTotals))
	for region, peers := range regionTotals {
		topRegions = append(topRegions, gin.H{
			"region": region,
			"peers":  peers,
		})
	}
	sort.Slice(topRegions, func(i, j int) bool {
		return topRegions[i]["peers"].(int) > topRegions[j]["peers"].(int)
	})
	if len(topRegions) > 6 {
		topRegions = topRegions[:6]
	}

	conversionRate := 0.0
	if totalUsers > 0 {
		conversionRate = (float64(activeSubscriptions) / float64(totalUsers)) * 100
	}

	autoRenewRate := 0.0
	if len(subscriptions) > 0 {
		autoRenewRate = (float64(autoRenewEnabled) / float64(len(subscriptions))) * 100
	}

	c.JSON(http.StatusOK, gin.H{
		"total_users":             totalUsers,
		"active_subscriptions":    activeSubscriptions,
		"expired_subscriptions":   expiredSubscriptions,
		"cancelled_subscriptions": cancelledSubscriptions,
		"active_vpn_keys":         activeKeys,
		"total_servers":           totalServers,
		"active_servers":          activeServers,
		"pending_payments":        pendingPayments,
		"total_revenue":           totalRevenue,
		"monthly_revenue":         monthlyRevenue,
		"window_days":             windowDays,
		"conversion_rate":         conversionRate,
		"auto_renew_rate":         autoRenewRate,
		"users_series":            usersSeries,
		"revenue_series":          revenueSeries,
		"subscriptions_by_plan":   planBreakdown,
		"subscriptions_by_status": statusBreakdown,
		"top_regions":             topRegions,
		"server_load":             serverLoad,
	})
}

func maxInt(a, b int) int {
	if a > b {
		return a
	}
	return b
}

// GET /api/v1/admin/export/:entity
func (h *AdminHandler) ExportCSV(c *gin.Context) {
	entity := c.Param("entity")

	var buf bytes.Buffer
	writer := csv.NewWriter(&buf)

	switch entity {
	case "users":
		_ = writer.Write([]string{"id", "email", "role", "created_at"})
		var users []models.User
		h.db.Order("id asc").Find(&users)
		for _, u := range users {
			_ = writer.Write([]string{
				fmt.Sprint(u.ID),
				u.Email,
				string(u.Role),
				u.CreatedAt.Format(time.RFC3339),
			})
		}
	case "payments":
		_ = writer.Write([]string{"id", "user_id", "amount", "currency", "status", "plan", "created_at", "confirmed_at"})
		var payments []models.Payment
		h.db.Order("id desc").Find(&payments)
		for _, p := range payments {
			confirmedAt := ""
			if p.ConfirmedAt != nil {
				confirmedAt = p.ConfirmedAt.Format(time.RFC3339)
			}
			_ = writer.Write([]string{
				fmt.Sprint(p.ID),
				fmt.Sprint(p.UserID),
				fmt.Sprintf("%.2f", p.Amount),
				p.Currency,
				string(p.Status),
				string(p.Plan),
				p.CreatedAt.Format(time.RFC3339),
				confirmedAt,
			})
		}
	case "servers":
		_ = writer.Write([]string{"id", "name", "region", "country_code", "host", "endpoint", "active", "max_peers", "awg_port"})
		var servers []models.VPNServer
		h.db.Order("id asc").Find(&servers)
		for _, s := range servers {
			_ = writer.Write([]string{
				fmt.Sprint(s.ID),
				s.Name,
				s.Region,
				s.CountryCode,
				s.Host,
				s.Endpoint,
				fmt.Sprint(s.Active),
				fmt.Sprint(s.MaxPeers),
				fmt.Sprint(s.AWGPort),
			})
		}
	default:
		c.JSON(http.StatusBadRequest, gin.H{"error": "unknown export entity"})
		return
	}

	writer.Flush()
	if err := writer.Error(); err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "failed to build csv"})
		return
	}

	c.Header("Content-Type", "text/csv; charset=utf-8")
	c.Header("Content-Disposition", fmt.Sprintf("attachment; filename=%s-export.csv", entity))
	c.String(http.StatusOK, buf.String())
}

// POST /api/v1/admin/users/:id/upgrade
func (h *AdminHandler) UpgradeUser(c *gin.Context) {
	userId := c.Param("id")
	var user models.User
	if err := h.db.First(&user, userId).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "user not found"})
		return
	}
	var sub models.Subscription
	h.db.Where("user_id = ?", user.ID).FirstOrInit(&sub, models.Subscription{UserID: user.ID})
	sub.Status = models.SubActive
	sub.Plan = "basic"
	sub.ExpiresAt = time.Now().AddDate(0, 1, 0) // +1 month
	h.db.Save(&sub)
	c.JSON(http.StatusOK, gin.H{"message": "user upgraded to basic"})
}

// POST /api/v1/admin/users/:id/revoke
func (h *AdminHandler) RevokeUserKeys(c *gin.Context) {
	userId := c.Param("id")
	now := time.Now()
	// Mark all keys as revoked
	var keys []models.VPNKey
	h.db.Where("user_id = ? AND revoked_at IS NULL", userId).Preload("Server").Find(&keys)

	for _, k := range keys {
		if k.PublicKey != "" {
			if err := removeAWGPeer(&k.Server, k.PublicKey); err != nil {
				fmt.Printf("[WARN] Admin RevokeUserKeys SSH failed for %s: %v\n", k.Server.Name, err)
			}
		}
	}

	h.db.Model(&models.VPNKey{}).Where("user_id = ? AND revoked_at IS NULL", userId).Update("revoked_at", &now)
	c.JSON(http.StatusOK, gin.H{"message": "all user keys revoked"})
}

// POST /api/v1/admin/users/:id/set-role
func (h *AdminHandler) SetUserRole(c *gin.Context) {
	id := c.Param("id")

	var req struct {
		Role string `json:"role" binding:"required,oneof=user admin"`
	}
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}

	// Prevent removing the last admin
	if req.Role == "user" {
		var adminCount int64
		h.db.Model(&models.User{}).Where("role = ?", models.RoleAdmin).Count(&adminCount)
		if adminCount <= 1 {
			c.JSON(http.StatusConflict, gin.H{"error": "Нельзя снять роль с последнего администратора"})
			return
		}
	}

	var u models.User
	if err := h.db.First(&u, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "user not found"})
		return
	}

	h.db.Model(&u).Update("role", req.Role)
	c.JSON(http.StatusOK, gin.H{"id": u.ID, "role": req.Role})
}

// POST /api/v1/admin/servers/:id/toggle
func (h *AdminHandler) ToggleServer(c *gin.Context) {
	id := c.Param("id")
	var s models.VPNServer
	if err := h.db.First(&s, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "server not found"})
		return
	}
	s.Active = !s.Active
	h.db.Save(&s)
	c.JSON(http.StatusOK, gin.H{"message": "server toggled", "active": s.Active})
}

// POST /api/v1/admin/payments/:id/approve
func (h *AdminHandler) ApprovePayment(c *gin.Context) {
	id := c.Param("id")
	var p models.Payment
	if err := h.db.First(&p, id).Error; err != nil {
		c.JSON(http.StatusNotFound, gin.H{"error": "payment not found"})
		return
	}
	p.Status = "succeeded"
	now := time.Now()
	p.ConfirmedAt = &now
	h.db.Save(&p)

	// Upgrade user subscription immediately
	var sub models.Subscription
	h.db.Where("user_id = ?", p.UserID).FirstOrInit(&sub, models.Subscription{UserID: p.UserID})

	now = time.Now()
	var durationDays int
	if p.Plan == models.PlanTrial {
		durationDays = 7
	} else {
		durationDays = 30 // для Basic
	}

	newExpiry := now.AddDate(0, 0, durationDays)
	if p.Plan != models.PlanTrial && sub.ExpiresAt.After(now) && sub.Plan != models.PlanFree {
		newExpiry = sub.ExpiresAt.AddDate(0, 0, durationDays)
	}

	sub.Status = models.SubActive
	sub.Plan = p.Plan
	sub.ExpiresAt = newExpiry
	h.db.Save(&sub)

	c.JSON(http.StatusOK, gin.H{"message": "payment manually approved and subscription issued"})
}
