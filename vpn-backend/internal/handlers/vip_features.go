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
	legacyVIPRoutingProfileName = "RU without VPN"
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

type defaultRoutingProfileSeed struct {
	Code             string
	Name             string
	Action           models.RoutingProfileAction
	Description      string
	Icon             string
	SortOrder        int
	EnabledByDefault bool
	Domains          []string
	DomainSuffixes   []string
	CIDRs            []string
	LegacyNames      []string
}

func defaultRoutingProfileSeeds() []defaultRoutingProfileSeed {
	return []defaultRoutingProfileSeed{
		{
			Code:             "ru_direct",
			Name:             "RU без VPN",
			Action:           models.RoutingProfileDirect,
			Description:      "Базовый системный пресет для .ru, .xn--p1ai и локальных российских ресурсов.",
			Icon:             "map-pin.svg",
			SortOrder:        10,
			EnabledByDefault: true,
			DomainSuffixes:   []string{".ru", ".xn--p1ai"},
			LegacyNames:      []string{legacyVIPRoutingProfileName},
		},
		{
			Code:             "banks_gosuslugi_direct",
			Name:             "Банки и госуслуги",
			Action:           models.RoutingProfileDirect,
			Description:      "Госуслуги, налоги и популярные банковские домены идут напрямую без VPN.",
			Icon:             "shield-tick.svg",
			SortOrder:        20,
			EnabledByDefault: false,
			Domains: []string{
				"gosuslugi.ru",
				"esia.gosuslugi.ru",
				"gosuslugi.com",
			},
			DomainSuffixes: []string{
				".gosuslugi.ru",
				".nalog.gov.ru",
				".sberbank.ru",
				".sber.ru",
				".vtb.ru",
				".alfabank.ru",
				".tbank.ru",
			},
		},
		{
			Code:             "games_launchers_direct",
			Name:             "Игры и лаунчеры без VPN",
			Action:           models.RoutingProfileDirect,
			Description:      "Локальные игровые лаунчеры и сервисы можно держать вне VPN для более ровного пинга.",
			Icon:             "gauge.svg",
			SortOrder:        30,
			EnabledByDefault: false,
			DomainSuffixes: []string{
				".vkplay.ru",
				".4game.ru",
				".lesta.ru",
				".tanki.su",
				".mail.ru",
			},
		},
		{
			Code:             "local_media_direct",
			Name:             "Локальные медиа без VPN",
			Action:           models.RoutingProfileDirect,
			Description:      "Российские видеосервисы и локальные стриминги идут напрямую.",
			Icon:             "monitor.svg",
			SortOrder:        40,
			EnabledByDefault: false,
			DomainSuffixes: []string{
				".kinopoisk.ru",
				".rutube.ru",
				".ivi.ru",
				".okko.tv",
				".wink.ru",
				".smotrim.ru",
				".premier.one",
			},
		},
		{
			Code:             "ai_proxy",
			Name:             "AI через VPN",
			Action:           models.RoutingProfileProxy,
			Description:      "ChatGPT, Claude, Gemini, Perplexity и другие AI-сервисы принудительно идут через VPN.",
			Icon:             "scan-line.svg",
			SortOrder:        110,
			EnabledByDefault: false,
			Domains: []string{
				"chatgpt.com",
				"claude.ai",
				"perplexity.ai",
				"poe.com",
				"notebooklm.google.com",
				"gemini.google.com",
			},
			DomainSuffixes: []string{
				".openai.com",
				".anthropic.com",
				".perplexity.ai",
				".poe.com",
				".openrouter.ai",
				".generativelanguage.googleapis.com",
				".notebooklm.google.com",
				".gemini.google.com",
			},
		},
		{
			Code:             "media_proxy",
			Name:             "Медиа через VPN",
			Action:           models.RoutingProfileProxy,
			Description:      "YouTube, Twitch, TikTok и связанные медиадомены идут через VPN без ручной настройки.",
			Icon:             "monitor.svg",
			SortOrder:        120,
			EnabledByDefault: false,
			Domains: []string{
				"youtube.com",
				"youtu.be",
				"twitch.tv",
				"tiktok.com",
			},
			DomainSuffixes: []string{
				".youtube.com",
				".youtu.be",
				".googlevideo.com",
				".ytimg.com",
				".youtubei.googleapis.com",
				".twitch.tv",
				".ttvnw.net",
				".jtvnw.net",
				".tiktok.com",
				".tiktokcdn.com",
				".byteoversea.com",
			},
		},
	}
}

func ensureRoutingProfileSchema(db *gorm.DB) error {
	requiredColumns := []string{"Code", "Action", "Description", "Icon", "SortOrder"}
	for _, column := range requiredColumns {
		if db.Migrator().HasColumn(&models.RoutingProfile{}, column) {
			continue
		}
		if err := db.Migrator().AddColumn(&models.RoutingProfile{}, column); err != nil {
			return err
		}
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("coalesce(action, '') = ''").
		UpdateColumn("action", models.RoutingProfileDirect).Error; err != nil {
		return err
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("description IS NULL").
		UpdateColumn("description", "").Error; err != nil {
		return err
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("icon IS NULL").
		UpdateColumn("icon", "").Error; err != nil {
		return err
	}

	if err := db.Model(&models.RoutingProfile{}).
		Where("sort_order IS NULL").
		UpdateColumn("sort_order", 0).Error; err != nil {
		return err
	}

	return nil
}

func ensureDefaultRoutingProfiles(db *gorm.DB, userID uint) error {
	if err := ensureRoutingProfileSchema(db); err != nil {
		return err
	}

	for _, seed := range defaultRoutingProfileSeeds() {
		var profile models.RoutingProfile
		err := db.Where("user_id = ? AND kind = ? AND code = ?", userID, models.RoutingProfileSystem, seed.Code).
			First(&profile).Error
		if err != nil && err != gorm.ErrRecordNotFound {
			return err
		}

		if err == gorm.ErrRecordNotFound {
			for _, legacyName := range seed.LegacyNames {
				legacyErr := db.Where("user_id = ? AND kind = ? AND name = ?", userID, models.RoutingProfileSystem, legacyName).
					First(&profile).Error
				if legacyErr == nil {
					err = nil
					break
				}
				if legacyErr != nil && legacyErr != gorm.ErrRecordNotFound {
					return legacyErr
				}
			}
		}

		domainsJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.Domains))
		domainSuffixesJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.DomainSuffixes))
		cidrsJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.CIDRs))

		if err == gorm.ErrRecordNotFound {
			profile = models.RoutingProfile{
				UserID:             userID,
				Name:               seed.Name,
				Code:               seed.Code,
				Kind:               models.RoutingProfileSystem,
				Action:             seed.Action,
				Enabled:            seed.EnabledByDefault,
				Description:        seed.Description,
				Icon:               seed.Icon,
				SortOrder:          seed.SortOrder,
				DomainsJSON:        string(domainsJSON),
				DomainSuffixesJSON: string(domainSuffixesJSON),
				CIDRsJSON:          string(cidrsJSON),
			}
			if createErr := db.Create(&profile).Error; createErr != nil {
				return createErr
			}
			continue
		}

		updates := map[string]interface{}{
			"name":                 seed.Name,
			"code":                 seed.Code,
			"action":               seed.Action,
			"description":          seed.Description,
			"icon":                 seed.Icon,
			"sort_order":           seed.SortOrder,
			"domains_json":         string(domainsJSON),
			"domain_suffixes_json": string(domainSuffixesJSON),
			"cidrs_json":           string(cidrsJSON),
		}
		if updateErr := db.Model(&profile).Updates(updates).Error; updateErr != nil {
			return updateErr
		}
	}

	return nil
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
		"code":            profile.Code,
		"kind":            profile.Kind,
		"action":          profile.Action,
		"enabled":         profile.Enabled,
		"description":     profile.Description,
		"icon":            profile.Icon,
		"sort_order":      profile.SortOrder,
		"editable":        profile.Kind != models.RoutingProfileSystem,
		"deletable":       profile.Kind != models.RoutingProfileSystem,
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
