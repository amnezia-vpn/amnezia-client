package models

import (
	"time"

	"gorm.io/gorm"
)

type Role string

const (
	RoleUser  Role = "user"
	RoleAdmin Role = "admin"
)

type User struct {
	gorm.Model
	Email        string `gorm:"uniqueIndex;not null"`
	PasswordHash string `gorm:"not null"`
	Role         Role   `gorm:"default:user"`

	Subscription *Subscription
	VPNKeys      []VPNKey
	Payments     []Payment
}

type PlanType string

const (
	PlanFree  PlanType = "free"
	PlanTrial PlanType = "trial"
	PlanBasic PlanType = "basic"
)

type SubscriptionStatus string

const (
	SubActive    SubscriptionStatus = "active"
	SubExpired   SubscriptionStatus = "expired"
	SubCancelled SubscriptionStatus = "cancelled"
)

type Subscription struct {
	gorm.Model
	UserID          uint               `gorm:"uniqueIndex;not null"`
	Plan            PlanType           `gorm:"default:free"`
	Status          SubscriptionStatus `gorm:"default:active"`
	ExpiresAt       time.Time
	AutoRenew       bool   `gorm:"default:true"`
	PaymentMethodID string `gorm:"default:''"` // YooKassa payment_method_id для автосписания
}

type VPNServer struct {
	gorm.Model
	Name      string `gorm:"not null"`
	Host      string `gorm:"not null"`
	PublicKey string `gorm:"not null"`
	MaxPeers  int    `gorm:"default:100"`
	Region      string
	CountryCode string `gorm:"default:''"` // ISO 3166-1 alpha-2, e.g. "RU", "US", "DE"
	Active      bool   `gorm:"default:true"`

	// FBLinkWG 2 параметры (обфускация)
	AWGPort  int    `gorm:"default:51820"`
	Endpoint string // host:port для клиентов (может отличаться от Host)
	MTU      string `gorm:"default:1280"`
	Jc       string `gorm:"default:4"`
	Jmin     string `gorm:"default:40"`
	Jmax     string `gorm:"default:70"`
	S1       string `gorm:"default:0"`
	S2       string `gorm:"default:0"`
	S3       string `gorm:"default:0"`          // AWG2: cookie reply
	S4       string `gorm:"default:0"`          // AWG2: transport
	H1       string `gorm:"not null;default:0"` // range: min-max или фиксированное
	H2       string `gorm:"not null;default:0"`
	H3       string `gorm:"not null;default:0"`
	H4       string `gorm:"not null;default:0"`
	I1       string
	I2       string
	I3       string
	I4       string
	I5       string

	// SSH доступ для управления peers
	SSHHost      string `gorm:"default:''"` // IP для SSH (если отличается от Host)
	SSHPort      int    `gorm:"default:22"`
	SSHUser      string `gorm:"default:'root'"`
	SSHPassword  string // пароль SSH (ili private key)
	AWGContainer string `gorm:"default:'fblink-awg2'"`
	AWGInterface string `gorm:"default:'awg0'"`

	VPNKeys []VPNKey `gorm:"foreignKey:ServerID"`
}

type VPNKey struct {
	gorm.Model
	UserID       uint   `gorm:"not null"`
	ServerID     uint   `gorm:"not null"`
	PublicKey    string `gorm:"not null"`
	PresharedKey string // Уникальный PSK для каждого клиента (безопасность)
	ConfigText   string `gorm:"not null"`
	IssuedAt     time.Time
	RevokedAt    *time.Time

	User   User
	Server VPNServer `gorm:"foreignKey:ServerID"`
}

type PaymentStatus string

const (
	PaymentPending   PaymentStatus = "pending"
	PaymentSucceeded PaymentStatus = "succeeded"
	PaymentCancelled PaymentStatus = "cancelled"
)

type Payment struct {
	gorm.Model
	UserID      uint          `gorm:"not null"`
	YooKassaID  string        `gorm:"uniqueIndex"`
	Amount      float64       `gorm:"not null"`
	Currency    string        `gorm:"default:RUB"`
	Status      PaymentStatus `gorm:"default:pending"`
	Plan        PlanType      `gorm:"not null"`
	ConfirmURL  string        // URL для оплаты (от ЮKassa)
	ConfirmedAt *time.Time

	User User
}

type VerificationCode struct {
	gorm.Model
	Email     string    `gorm:"not null;index"`
	Code      string    `gorm:"not null"`
	Purpose   string    `gorm:"not null"` // "verify" | "reset"
	ExpiresAt time.Time `gorm:"not null"`
	Used      bool      `gorm:"default:false"`
}
