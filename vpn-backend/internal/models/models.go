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
	PlanFree     PlanType = "free"
	PlanTrial    PlanType = "trial"
	PlanBasic    PlanType = "basic"
	PlanBasic3M  PlanType = "basic_3m"
	PlanVIP      PlanType = "vip"
	PlanVIP3M    PlanType = "vip_3m"
)

type SubscriptionStatus string

const (
	SubActive    SubscriptionStatus = "active"
	SubExpired   SubscriptionStatus = "expired"
	SubCancelled SubscriptionStatus = "cancelled"
)

type Subscription struct {
	gorm.Model
	UserID            uint               `gorm:"uniqueIndex;not null"`
	Plan              PlanType           `gorm:"default:free"`
	Status            SubscriptionStatus `gorm:"default:active"`
	ExpiresAt         time.Time
	AutoRenew         bool   `gorm:"default:true"`
	PaymentMethodID   string `gorm:"default:''"` // YooKassa payment_method_id для автосписания
	VIPAdBlockEnabled bool   `gorm:"column:vip_ad_block_enabled;default:false"`
}

type VPNServer struct {
	gorm.Model
	Name        string `gorm:"not null"`
	Host        string `gorm:"not null"`
	PublicKey   string `gorm:"not null"`
	MaxPeers    int    `gorm:"default:100"`
	Region      string
	CountryCode string `gorm:"default:''"` // ISO 3166-1 alpha-2, e.g. "RU", "US", "DE"
	Active      bool   `gorm:"default:true"`
	VIPOnly     bool   `gorm:"column:vip_only;default:false"`

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
	AWGContainer string `gorm:"default:'amnezia-awg2'"`
	AWGInterface string `gorm:"default:'awg0'"`

	// Pi-hole AdBlock
	PiHoleMode          string `gorm:"default:'auto'"` // auto | host | docker | disabled
	PiHoleContainerName string `gorm:"default:''"`     // override docker container name
	PiHoleGroupName     string `gorm:"default:'VIP'"`  // gravity.db group name
	PiHoleEnabled       bool   `gorm:"default:true"`
	PiHoleDNSIP         string `gorm:"default:''"` // кеш IP после pihole-sync (не ходим по SSH при каждом запросе)
	PiHoleLastSyncAt    *time.Time
	PiHoleLastSyncError string `gorm:"default:''"`
	PiHoleLastMode      string `gorm:"default:''"`
	PiHoleLastClientIP  string `gorm:"default:''"`

	VPNKeys       []VPNKey             `gorm:"foreignKey:ServerID"`
	VLESSTemplate *VLESSServerTemplate `gorm:"foreignKey:ServerID"`
	VLESSClients  []VLESSCredential    `gorm:"foreignKey:ServerID"`
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

type VLESSServerTemplate struct {
	gorm.Model
	ServerID      uint   `gorm:"uniqueIndex;not null"`
	ClientID      string `gorm:"default:''"`
	Address       string `gorm:"not null"`
	Port          int    `gorm:"default:443"`
	ServerName    string `gorm:"not null"`
	PublicKey     string `gorm:"not null"`
	ShortID       string `gorm:"not null"`
	Fingerprint   string `gorm:"default:'chrome'"`
	Flow          string `gorm:"default:'xtls-rprx-vision'"`
	Network       string `gorm:"default:'tcp'"`
	Security      string `gorm:"default:'reality'"`
	SpiderX       string `gorm:"default:''"`
	ContainerName string `gorm:"default:'amnezia-xray'"`
}

type VLESSCredential struct {
	gorm.Model
	UserID    uint   `gorm:"not null;uniqueIndex:idx_vless_user_server"`
	ServerID  uint   `gorm:"not null;uniqueIndex:idx_vless_user_server"`
	ClientID  string `gorm:"not null"`
	RevokedAt *time.Time

	User   User
	Server VPNServer `gorm:"foreignKey:ServerID"`
}

type RoutingProfileKind string

const (
	RoutingProfileSystem RoutingProfileKind = "system"
	RoutingProfileCustom RoutingProfileKind = "custom"
)

type RoutingProfileAction string

const (
	RoutingProfileDirect RoutingProfileAction = "direct"
	RoutingProfileProxy  RoutingProfileAction = "proxy"
)

type RoutingProfile struct {
	gorm.Model
	UserID             uint                 `gorm:"not null;index"`
	Name               string               `gorm:"not null"`
	Code               string               `gorm:"default:'';index"`
	TemplateCode       string               `gorm:"default:'';index"`
	Kind               RoutingProfileKind   `gorm:"default:'custom'"`
	Action             RoutingProfileAction `gorm:"default:'direct'"`
	Enabled            bool                 `gorm:"default:false"`
	Description        string               `gorm:"default:''"`
	Icon               string               `gorm:"default:''"`
	SortOrder          int                  `gorm:"default:0"`
	DomainsJSON        string               `gorm:"column:domains_json;default:'[]'"`
	DomainSuffixesJSON string               `gorm:"column:domain_suffixes_json;default:'[]'"`
	CIDRsJSON          string               `gorm:"column:cidrs_json;default:'[]'"`

	User User
}

type PaymentStatus string

const (
	PaymentPending   PaymentStatus = "pending"
	PaymentSucceeded PaymentStatus = "succeeded"
	PaymentCancelled PaymentStatus = "cancelled"
)

type Payment struct {
	gorm.Model
	UserID         uint          `gorm:"not null"`
	YooKassaID     string        `gorm:"uniqueIndex"`
	Amount         float64       `gorm:"not null"`
	OriginalAmount float64       `gorm:"not null;default:0"`
	DiscountAmount float64       `gorm:"not null;default:0"`
	Currency       string        `gorm:"default:RUB"`
	Status         PaymentStatus `gorm:"default:pending"`
	Plan           PlanType      `gorm:"not null"`
	ConfirmURL     string // URL для оплаты (от ЮKassa)
	ConfirmedAt    *time.Time
	PromoCodeID    *uint

	User      User
	PromoCode *PromoCode
}

type PromoCode struct {
	gorm.Model
	Code            string `gorm:"uniqueIndex;not null"`
	Description     string `gorm:"default:''"`
	DiscountPercent int    `gorm:"not null"`
	MaxUses         int    `gorm:"default:0"`
	UsedCount       int    `gorm:"default:0"`
	Active          bool   `gorm:"default:true"`
	ApplicablePlans string `gorm:"default:'all'"`
	OncePerUser     bool   `gorm:"default:true"`
	ExpiresAt       *time.Time

	Payments []Payment
}

type VerificationCode struct {
	gorm.Model
	Email     string    `gorm:"not null;index"`
	Code      string    `gorm:"not null"`
	Purpose   string    `gorm:"not null"` // "verify" | "reset"
	ExpiresAt time.Time `gorm:"not null"`
	Used      bool      `gorm:"default:false"`
}
