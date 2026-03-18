package handlers

import (
	"crypto/tls"
	"fmt"
	"log"
	"net"
	"net/smtp"
	"strings"
	"time"
	"vpn-backend/internal/config"
)

const smtpTimeout = 15 * time.Second

// sendVerificationEmailAsync отправляет email в горутине (не блокирует UI)
func sendVerificationEmailAsync(cfg *config.Config, toEmail, code, purpose string) {
	go func() {
		if err := sendVerificationEmail(cfg, toEmail, code, purpose); err != nil {
			log.Printf("[ERROR] Failed to send %s email to %s: %v", purpose, toEmail, err)
		} else {
			log.Printf("[EMAIL] %s code sent to %s", purpose, toEmail)
		}
	}()
}

func sendVerificationEmail(cfg *config.Config, toEmail, code, purpose string) error {
	if cfg.SMTPHost == "" {
		return fmt.Errorf("SMTP not configured")
	}

	from := cfg.SMTPUser
	displayFrom := cfg.SMTPFrom
	if displayFrom == "" {
		displayFrom = from
	}

	subject := "FBLink VPN — код подтверждения"
	body := fmt.Sprintf("Ваш код подтверждения: %s\n\nКод действителен 10 минут.", code)
	if purpose == "reset" {
		subject = "FBLink VPN — восстановление пароля"
		body = fmt.Sprintf("Ваш код для сброса пароля: %s\n\nКод действителен 10 минут.\n\nЕсли вы не запрашивали сброс пароля, проигнорируйте это письмо.", code)
	}

	msg := strings.Join([]string{
		"From: " + displayFrom,
		"To: " + toEmail,
		"Subject: " + subject,
		"MIME-Version: 1.0",
		"Content-Type: text/plain; charset=UTF-8",
		"",
		body,
	}, "\r\n")

	addr := net.JoinHostPort(cfg.SMTPHost, fmt.Sprintf("%d", cfg.SMTPPort))

	conn, err := net.DialTimeout("tcp", addr, smtpTimeout)
	if err != nil {
		return fmt.Errorf("SMTP dial: %w", err)
	}

	// Устанавливаем дедлайн ДО создания SMTP клиента, чтобы он применялся ко всем операциям
	if tc, ok := conn.(*net.TCPConn); ok {
		tc.SetDeadline(time.Now().Add(30 * time.Second))
	}

	client, err := smtp.NewClient(conn, cfg.SMTPHost)
	if err != nil {
		conn.Close()
		return fmt.Errorf("SMTP client: %w", err)
	}
	defer client.Close()

	tlsConfig := &tls.Config{ServerName: cfg.SMTPHost}
	if err := client.StartTLS(tlsConfig); err != nil {
		return fmt.Errorf("STARTTLS: %w", err)
	}

	auth := smtp.PlainAuth("", cfg.SMTPUser, cfg.SMTPPassword, cfg.SMTPHost)
	if err := client.Auth(auth); err != nil {
		return fmt.Errorf("SMTP auth: %w", err)
	}

	if err := client.Mail(from); err != nil {
		return fmt.Errorf("SMTP MAIL FROM: %w", err)
	}
	if err := client.Rcpt(toEmail); err != nil {
		return fmt.Errorf("SMTP RCPT TO: %w", err)
	}
	w, err := client.Data()
	if err != nil {
		return fmt.Errorf("SMTP DATA: %w", err)
	}
	if _, err := w.Write([]byte(msg)); err != nil {
		return fmt.Errorf("SMTP write: %w", err)
	}
	if err := w.Close(); err != nil {
		return fmt.Errorf("SMTP close data: %w", err)
	}

	return client.Quit()
}
