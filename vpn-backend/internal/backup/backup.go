package backup

import (
	"bytes"
	"encoding/base64"
	"encoding/csv"
	"fmt"
	"io"
	"log"
	"mime/multipart"
	"net/smtp"
	"net/textproto"
	"os"
	"strconv"
	"time"

	"vpn-backend/internal/config"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

// RunScheduler запускает периодический бэкап. Вызывать как горутину из main.
func RunScheduler(db *gorm.DB, cfg *config.Config) {
	if cfg.SMTPHost == "" {
		log.Println("[backup] SMTP_HOST не настроен — планировщик бэкапов отключён")
		return
	}

	interval := time.Duration(cfg.BackupIntervalHours) * time.Hour
	log.Printf("[backup] Планировщик запущен, интервал: %v", interval)

	ticker := time.NewTicker(interval)
	defer ticker.Stop()

	for range ticker.C {
		if err := DoBackup(db, cfg); err != nil {
			log.Printf("[backup] Ошибка бэкапа: %v", err)
		}
	}
}

// DoBackup выполняет один бэкап: читает БД, генерирует CSV, отправляет всем админам.
func DoBackup(db *gorm.DB, cfg *config.Config) error {
	log.Println("[backup] Начало бэкапа...")

	// 1. Email-адреса всех администраторов
	var admins []models.User
	if err := db.Where("role = ?", models.RoleAdmin).Find(&admins).Error; err != nil {
		return fmt.Errorf("не удалось получить список админов: %w", err)
	}
	if len(admins) == 0 {
		log.Println("[backup] Нет ни одного администратора в БД — пропускаем отправку")
		return nil
	}
	log.Printf("[backup] Найдено %d администратор(ов): отправка на %v", len(admins), func() []string {
		emails := make([]string, len(admins))
		for i, a := range admins {
			emails[i] = a.Email
		}
		return emails
	}())
	adminEmails := make([]string, 0, len(admins))
	for _, a := range admins {
		adminEmails = append(adminEmails, a.Email)
	}

	// 2. Читаем файл SQLite как бинарный бэкап
	dbBytes, err := os.ReadFile(cfg.DBPath)
	if err != nil {
		return fmt.Errorf("не удалось прочитать файл БД: %w", err)
	}

	// 3. CSV-таблицы
	type csvFile struct {
		name string
		data []byte
	}
	csvFiles := []csvFile{}

	generators := []struct {
		name string
		fn   func(*gorm.DB) ([]byte, error)
	}{
		{"users.csv", genUsersCSV},
		{"subscriptions.csv", genSubscriptionsCSV},
		{"payments.csv", genPaymentsCSV},
		{"servers.csv", genServersCSV},
		{"vpn_keys.csv", genVPNKeysCSV},
	}
	for _, g := range generators {
		data, err := g.fn(db)
		if err != nil {
			return fmt.Errorf("генерация %s: %w", g.name, err)
		}
		csvFiles = append(csvFiles, csvFile{g.name, data})
	}

	// 4. Собираем вложения: db-файл + CSV
	ts := time.Now().Format("2006-01-02_15-04")
	attachments := map[string][]byte{
		fmt.Sprintf("vpn_backup_%s.db", ts): dbBytes,
	}
	for _, f := range csvFiles {
		attachments[f.name] = f.data
	}

	subject := fmt.Sprintf("Dr.Frake VPN — резервная копия БД (%s)", ts)
	body := fmt.Sprintf(
		"Автоматический бэкап базы данных от %s.\n\nАдминистраторы получателей: %v\n\nВложения:\n"+
			"  • vpn_backup_%s.db — полная копия SQLite\n"+
			"  • users.csv\n  • subscriptions.csv\n  • payments.csv\n  • servers.csv\n  • vpn_keys.csv",
		ts, adminEmails, ts,
	)

	if err := sendEmail(cfg, adminEmails, subject, body, attachments); err != nil {
		return fmt.Errorf("отправка email: %w", err)
	}

	log.Printf("[backup] Бэкап отправлен %d администратор(ам): %v", len(adminEmails), adminEmails)
	return nil
}

// ─── Генераторы CSV ──────────────────────────────────────────────────────────

func genUsersCSV(db *gorm.DB) ([]byte, error) {
	var users []models.User
	db.Find(&users)

	var buf bytes.Buffer
	w := csv.NewWriter(&buf)
	_ = w.Write([]string{"id", "email", "role", "created_at", "deleted_at"})
	for _, u := range users {
		deletedAt := ""
		if u.DeletedAt.Valid {
			deletedAt = u.DeletedAt.Time.Format(time.RFC3339)
		}
		_ = w.Write([]string{
			strconv.Itoa(int(u.ID)),
			u.Email,
			string(u.Role),
			u.CreatedAt.Format(time.RFC3339),
			deletedAt,
		})
	}
	w.Flush()
	return buf.Bytes(), w.Error()
}

func genSubscriptionsCSV(db *gorm.DB) ([]byte, error) {
	var subs []models.Subscription
	db.Find(&subs)

	var buf bytes.Buffer
	w := csv.NewWriter(&buf)
	_ = w.Write([]string{"id", "user_id", "plan", "status", "expires_at", "created_at"})
	for _, s := range subs {
		_ = w.Write([]string{
			strconv.Itoa(int(s.ID)),
			strconv.Itoa(int(s.UserID)),
			string(s.Plan),
			string(s.Status),
			s.ExpiresAt.Format(time.RFC3339),
			s.CreatedAt.Format(time.RFC3339),
		})
	}
	w.Flush()
	return buf.Bytes(), w.Error()
}

func genPaymentsCSV(db *gorm.DB) ([]byte, error) {
	var payments []models.Payment
	db.Find(&payments)

	var buf bytes.Buffer
	w := csv.NewWriter(&buf)
	_ = w.Write([]string{"id", "user_id", "yookassa_id", "amount", "currency", "status", "plan", "confirmed_at", "created_at"})
	for _, p := range payments {
		confirmedAt := ""
		if p.ConfirmedAt != nil {
			confirmedAt = p.ConfirmedAt.Format(time.RFC3339)
		}
		_ = w.Write([]string{
			strconv.Itoa(int(p.ID)),
			strconv.Itoa(int(p.UserID)),
			p.YooKassaID,
			fmt.Sprintf("%.2f", p.Amount),
			p.Currency,
			string(p.Status),
			string(p.Plan),
			confirmedAt,
			p.CreatedAt.Format(time.RFC3339),
		})
	}
	w.Flush()
	return buf.Bytes(), w.Error()
}

func genServersCSV(db *gorm.DB) ([]byte, error) {
	var servers []models.VPNServer
	db.Find(&servers)

	var buf bytes.Buffer
	w := csv.NewWriter(&buf)
	_ = w.Write([]string{"id", "name", "host", "endpoint", "region", "active", "max_peers", "awg_port", "created_at"})
	for _, s := range servers {
		_ = w.Write([]string{
			strconv.Itoa(int(s.ID)),
			s.Name,
			s.Host,
			s.Endpoint,
			s.Region,
			strconv.FormatBool(s.Active),
			strconv.Itoa(s.MaxPeers),
			strconv.Itoa(s.AWGPort),
			s.CreatedAt.Format(time.RFC3339),
		})
	}
	w.Flush()
	return buf.Bytes(), w.Error()
}

func genVPNKeysCSV(db *gorm.DB) ([]byte, error) {
	var keys []models.VPNKey
	db.Find(&keys)

	var buf bytes.Buffer
	w := csv.NewWriter(&buf)
	_ = w.Write([]string{"id", "user_id", "server_id", "issued_at", "revoked_at"})
	for _, k := range keys {
		revokedAt := ""
		if k.RevokedAt != nil {
			revokedAt = k.RevokedAt.Format(time.RFC3339)
		}
		_ = w.Write([]string{
			strconv.Itoa(int(k.ID)),
			strconv.Itoa(int(k.UserID)),
			strconv.Itoa(int(k.ServerID)),
			k.IssuedAt.Format(time.RFC3339),
			revokedAt,
		})
	}
	w.Flush()
	return buf.Bytes(), w.Error()
}

// ─── Отправка email с вложениями ─────────────────────────────────────────────

func sendEmail(cfg *config.Config, to []string, subject, body string, attachments map[string][]byte) error {
	from := cfg.SMTPFrom
	if from == "" {
		from = cfg.SMTPUser
	}

	// Строим MIME-сообщение
	var mixedBuf bytes.Buffer
	mw := multipart.NewWriter(&mixedBuf)

	// Текстовая часть
	textHeader := make(textproto.MIMEHeader)
	textHeader.Set("Content-Type", "text/plain; charset=utf-8")
	textPart, err := mw.CreatePart(textHeader)
	if err != nil {
		return err
	}
	if _, err = io.WriteString(textPart, body); err != nil {
		return err
	}

	// Вложения
	for filename, data := range attachments {
		ct := "application/octet-stream"
		if len(filename) > 4 && filename[len(filename)-4:] == ".csv" {
			ct = "text/csv; charset=utf-8"
		}
		ah := make(textproto.MIMEHeader)
		ah.Set("Content-Type", ct)
		ah.Set("Content-Transfer-Encoding", "base64")
		ah.Set("Content-Disposition", fmt.Sprintf(`attachment; filename="%s"`, filename))

		ap, err := mw.CreatePart(ah)
		if err != nil {
			return err
		}
		encoded := base64.StdEncoding.EncodeToString(data)
		for i := 0; i < len(encoded); i += 76 {
			end := i + 76
			if end > len(encoded) {
				end = len(encoded)
			}
			if _, err = fmt.Fprintf(ap, "%s\r\n", encoded[i:end]); err != nil {
				return err
			}
		}
	}
	mw.Close()

	// Заголовки письма
	var msg bytes.Buffer
	fmt.Fprintf(&msg, "From: %s\r\n", from)
	fmt.Fprintf(&msg, "To: ")
	for i, addr := range to {
		if i > 0 {
			fmt.Fprint(&msg, ", ")
		}
		fmt.Fprint(&msg, addr)
	}
	fmt.Fprint(&msg, "\r\n")
	fmt.Fprintf(&msg, "Subject: %s\r\n", subject)
	fmt.Fprintf(&msg, "MIME-Version: 1.0\r\n")
	fmt.Fprintf(&msg, "Content-Type: multipart/mixed; boundary=\"%s\"\r\n\r\n", mw.Boundary())

	fullMsg := append(msg.Bytes(), mixedBuf.Bytes()...)

	// SMTP envelope sender must be a plain email address (no display name)
	envelopeFrom := cfg.SMTPUser

	auth := smtp.PlainAuth("", cfg.SMTPUser, cfg.SMTPPassword, cfg.SMTPHost)
	addr := fmt.Sprintf("%s:%d", cfg.SMTPHost, cfg.SMTPPort)
	return smtp.SendMail(addr, auth, envelopeFrom, to, fullMsg)
}