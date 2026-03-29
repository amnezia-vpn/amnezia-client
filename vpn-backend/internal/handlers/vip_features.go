package handlers

import (
	"encoding/json"
	"sort"
	"strings"
	"time"
	"vpn-backend/internal/models"

	"gorm.io/gorm"
)

const (
	vipRoutingProfileName = "RU without VPN"
)

type subscriptionCapabilities struct {
	AllowedProtocols         []string `json:"allowed_protocols"`
	CanUseSiteSplitTunnel    bool     `json:"can_use_site_split_tunneling"`
	CanUseAppSplitTunnel     bool     `json:"can_use_app_split_tunneling"`
	CanManageRoutingProfiles bool     `json:"can_manage_routing_profiles"`
}

func isSubscriptionEntitled(sub models.Subscription) bool {
	return sub.Status == models.SubActive && time.Now().Before(sub.ExpiresAt.Add(24*time.Hour))
}

func buildSubscriptionCapabilities(sub models.Subscription) subscriptionCapabilities {
	if !isSubscriptionEntitled(sub) {
		return subscriptionCapabilities{AllowedProtocols: []string{}}
	}

	switch sub.Plan {
	case models.PlanTrial, models.PlanBasic:
		return subscriptionCapabilities{
			AllowedProtocols: []string{"awg"},
		}
	case models.PlanVIP:
		return subscriptionCapabilities{
			AllowedProtocols:         []string{"vless"},
			CanUseSiteSplitTunnel:    true,
			CanUseAppSplitTunnel:     true,
			CanManageRoutingProfiles: true,
		}
	default:
		return subscriptionCapabilities{AllowedProtocols: []string{}}
	}
}

func canManageVIPFeatures(sub models.Subscription) bool {
	return buildSubscriptionCapabilities(sub).CanManageRoutingProfiles
}

func ensureDefaultRoutingProfiles(db *gorm.DB, userID uint) error {
	var systemProfile models.RoutingProfile
	err := db.Where("user_id = ? AND kind = ? AND name = ?", userID, models.RoutingProfileSystem, vipRoutingProfileName).
		First(&systemProfile).Error
	if err == nil {
		// Respect the user's explicit enabled/disabled choice for the built-in profile.
		// We only auto-create the system profile once when it does not exist yet.
		return nil
	}
	if err != nil && err != gorm.ErrRecordNotFound {
		return err
	}

	domainSuffixes, _ := json.Marshal([]string{".ru"})
	profile := models.RoutingProfile{
		UserID:             userID,
		Name:               vipRoutingProfileName,
		Kind:               models.RoutingProfileSystem,
		Enabled:            true,
		DomainSuffixesJSON: string(domainSuffixes),
		DomainsJSON:        "[]",
		CIDRsJSON:          "[]",
	}
	return db.Create(&profile).Error
}

func decodeJSONStringArray(raw string) []string {
	if strings.TrimSpace(raw) == "" {
		return []string{}
	}
	var values []string
	if err := json.Unmarshal([]byte(raw), &values); err != nil {
		return []string{}
	}

	seen := map[string]struct{}{}
	normalized := make([]string, 0, len(values))
	for _, value := range values {
		value = strings.TrimSpace(value)
		if value == "" {
			continue
		}
		if _, ok := seen[value]; ok {
			continue
		}
		seen[value] = struct{}{}
		normalized = append(normalized, value)
	}
	sort.Strings(normalized)
	return normalized
}

func encodeJSONStringArray(values []string) string {
	normalized := decodeJSONStringArray(mustMarshalStringArray(values))
	data, _ := json.Marshal(normalized)
	return string(data)
}

func mustMarshalStringArray(values []string) string {
	data, _ := json.Marshal(values)
	return string(data)
}

func routingProfileToMap(profile models.RoutingProfile) map[string]interface{} {
	return map[string]interface{}{
		"id":              profile.ID,
		"name":            profile.Name,
		"kind":            profile.Kind,
		"enabled":         profile.Enabled,
		"domains":         decodeJSONStringArray(profile.DomainsJSON),
		"domain_suffixes": decodeJSONStringArray(profile.DomainSuffixesJSON),
		"cidrs":           decodeJSONStringArray(profile.CIDRsJSON),
		"created_at":      profile.CreatedAt,
		"updated_at":      profile.UpdatedAt,
	}
}

func normalizeRoutingProfileInput(values []string) []string {
	joined := mustMarshalStringArray(values)
	return decodeJSONStringArray(joined)
}
