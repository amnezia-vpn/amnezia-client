package handlers

import (
	"encoding/json"
	"reflect"
	"sort"
	"strings"
	"sync"
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
	CanUseAdBlock            bool     `json:"can_use_ad_block"`
}

func isSubscriptionEntitled(sub models.Subscription) bool {
	return sub.Status == models.SubActive && time.Now().Before(sub.ExpiresAt.Add(24*time.Hour))
}

func buildSubscriptionCapabilities(sub models.Subscription) subscriptionCapabilities {
	if !isSubscriptionEntitled(sub) {
		return subscriptionCapabilities{AllowedProtocols: []string{}}
	}

	switch sub.Plan {
	case models.PlanTrial, models.PlanBasic, models.PlanBasic3M:
		return subscriptionCapabilities{
			AllowedProtocols: []string{"awg"},
		}
	case models.PlanVIP, models.PlanVIP3M:
		return subscriptionCapabilities{
			AllowedProtocols:         []string{"vless"},
			CanUseSiteSplitTunnel:    false, // deprecated: site routing now only via VIP profiles
			CanUseAppSplitTunnel:     true,
			CanManageRoutingProfiles: true,
			CanUseAdBlock:            true,
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
			EnabledByDefault: false,
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

type routingProfileSchemaState struct {
	once sync.Once
	err  error
}

var routingProfileSchemaStateMu sync.Mutex
var routingProfileSchemaStates = map[uintptr]*routingProfileSchemaState{}

func routingProfileSchemaCacheKey(db *gorm.DB) (uintptr, bool) {
	if db == nil {
		return 0, false
	}
	sqlDB, err := db.DB()
	if err != nil || sqlDB == nil {
		return 0, false
	}
	return reflect.ValueOf(sqlDB).Pointer(), true
}

func ensureRoutingProfileSchema(db *gorm.DB) error {
	cacheKey, ok := routingProfileSchemaCacheKey(db)
	if !ok {
		return doEnsureRoutingProfileSchema(db)
	}

	routingProfileSchemaStateMu.Lock()
	state, exists := routingProfileSchemaStates[cacheKey]
	if !exists {
		state = &routingProfileSchemaState{}
		routingProfileSchemaStates[cacheKey] = state
	}
	routingProfileSchemaStateMu.Unlock()

	state.once.Do(func() {
		state.err = doEnsureRoutingProfileSchema(db)
	})
	return state.err
}

func doEnsureRoutingProfileSchema(db *gorm.DB) error {
	requiredColumns := []string{"Code", "TemplateCode", "Action", "Description", "Icon", "SortOrder"}
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

	if err := db.Model(&models.RoutingProfile{}).
		Where("template_code IS NULL").
		UpdateColumn("template_code", "").Error; err != nil {
		return err
	}

	return nil
}

func buildSeedRoutingJSON(seed defaultRoutingProfileSeed) (string, string, string) {
	domainsJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.Domains))
	domainSuffixesJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.DomainSuffixes))
	cidrsJSON, _ := json.Marshal(normalizeRoutingProfileInput(seed.CIDRs))
	return string(domainsJSON), string(domainSuffixesJSON), string(cidrsJSON)
}

func ensureDefaultRoutingProfiles(db *gorm.DB, userID uint) error {
	if err := ensureRoutingProfileSchema(db); err != nil {
		return err
	}

	var systemProfiles []models.RoutingProfile
	if err := db.Where("user_id = ? AND kind = ?", userID, models.RoutingProfileSystem).
		Find(&systemProfiles).Error; err != nil {
		return err
	}

	systemByCode := make(map[string]*models.RoutingProfile, len(systemProfiles))
	systemByName := make(map[string]*models.RoutingProfile, len(systemProfiles))
	for i := range systemProfiles {
		profile := &systemProfiles[i]
		code := strings.TrimSpace(profile.Code)
		if code != "" {
			systemByCode[code] = profile
		}
		name := strings.TrimSpace(profile.Name)
		if name != "" {
			systemByName[name] = profile
		}
	}

	for _, seed := range defaultRoutingProfileSeeds() {
		profile := systemByCode[seed.Code]
		if profile == nil {
			for _, legacyName := range seed.LegacyNames {
				if candidate, ok := systemByName[strings.TrimSpace(legacyName)]; ok {
					profile = candidate
					break
				}
			}
		}

		domainsJSON, domainSuffixesJSON, cidrsJSON := buildSeedRoutingJSON(seed)

		if profile == nil {
			newProfile := models.RoutingProfile{
				UserID:             userID,
				Name:               seed.Name,
				Code:               seed.Code,
				Kind:               models.RoutingProfileSystem,
				Action:             seed.Action,
				Enabled:            seed.EnabledByDefault,
				Description:        seed.Description,
				Icon:               seed.Icon,
				SortOrder:          seed.SortOrder,
				DomainsJSON:        domainsJSON,
				DomainSuffixesJSON: domainSuffixesJSON,
				CIDRsJSON:          cidrsJSON,
			}
			if err := db.Create(&newProfile).Error; err != nil {
				return err
			}
			systemByCode[seed.Code] = &newProfile
			continue
		}

		updates := make(map[string]interface{}, 10)
		if profile.Name != seed.Name {
			updates["name"] = seed.Name
		}
		if profile.Code != seed.Code {
			updates["code"] = seed.Code
		}
		if profile.Action != seed.Action {
			updates["action"] = seed.Action
		}
		// Preserve enabled=true on existing system profiles until migration
		// converts them into custom copies. This avoids dropping user intent.
		if seed.EnabledByDefault && !profile.Enabled {
			updates["enabled"] = true
		}
		if profile.Description != seed.Description {
			updates["description"] = seed.Description
		}
		if profile.Icon != seed.Icon {
			updates["icon"] = seed.Icon
		}
		if profile.SortOrder != seed.SortOrder {
			updates["sort_order"] = seed.SortOrder
		}
		if profile.DomainsJSON != domainsJSON {
			updates["domains_json"] = domainsJSON
		}
		if profile.DomainSuffixesJSON != domainSuffixesJSON {
			updates["domain_suffixes_json"] = domainSuffixesJSON
		}
		if profile.CIDRsJSON != cidrsJSON {
			updates["cidrs_json"] = cidrsJSON
		}
		if profile.TemplateCode != "" {
			updates["template_code"] = ""
		}

		if len(updates) == 0 {
			continue
		}

		if err := db.Model(&models.RoutingProfile{}).
			Where("id = ? AND user_id = ?", profile.ID, userID).
			Updates(updates).Error; err != nil {
			return err
		}
	}

	var enabledSystemProfile models.RoutingProfile
	if err := db.Select("id").
		Where("user_id = ? AND kind = ? AND enabled = ?", userID, models.RoutingProfileSystem, true).
		Limit(1).
		First(&enabledSystemProfile).Error; err != nil {
		if err == gorm.ErrRecordNotFound {
			return nil
		}
		return err
	}

	if err := migrateEnabledSystemProfilesToCustomCopies(db, userID); err != nil {
		return err
	}

	return nil
}

func ensureCustomRoutingProfileFromTemplate(db *gorm.DB, userID uint, template models.RoutingProfile, enabled bool) (models.RoutingProfile, bool, error) {
	var existing models.RoutingProfile
	if err := db.Where("user_id = ? AND kind = ? AND template_code = ?", userID, models.RoutingProfileCustom, template.Code).
		First(&existing).Error; err == nil {
		return existing, false, nil
	} else if err != gorm.ErrRecordNotFound {
		return models.RoutingProfile{}, false, err
	}

	copyProfile := models.RoutingProfile{
		UserID:             userID,
		Name:               template.Name,
		Code:               "",
		TemplateCode:       template.Code,
		Kind:               models.RoutingProfileCustom,
		Action:             template.Action,
		Enabled:            enabled,
		Description:        template.Description,
		Icon:               template.Icon,
		SortOrder:          template.SortOrder,
		DomainsJSON:        template.DomainsJSON,
		DomainSuffixesJSON: template.DomainSuffixesJSON,
		CIDRsJSON:          template.CIDRsJSON,
	}

	if err := db.Create(&copyProfile).Error; err != nil {
		return models.RoutingProfile{}, false, err
	}
	return copyProfile, true, nil
}

func migrateEnabledSystemProfilesToCustomCopies(db *gorm.DB, userID uint) error {
	var systemProfiles []models.RoutingProfile
	if err := db.Where("user_id = ? AND kind = ? AND enabled = ?", userID, models.RoutingProfileSystem, true).
		Order("sort_order asc, id asc").
		Find(&systemProfiles).Error; err != nil {
		return err
	}

	for _, systemProfile := range systemProfiles {
		if strings.TrimSpace(systemProfile.Code) == "" {
			continue
		}

		customProfile, _, err := ensureCustomRoutingProfileFromTemplate(db, userID, systemProfile, true)
		if err != nil {
			return err
		}
		if !customProfile.Enabled {
			if err := db.Model(&models.RoutingProfile{}).
				Where("id = ? AND user_id = ?", customProfile.ID, userID).
				Update("enabled", true).Error; err != nil {
				return err
			}
		}

		if err := db.Model(&models.RoutingProfile{}).
			Where("id = ? AND user_id = ?", systemProfile.ID, userID).
			Update("enabled", false).Error; err != nil {
			return err
		}
	}

	return nil
}

func decodeJSONStringArray(raw string) []string {
	trimmedRaw := strings.TrimSpace(raw)
	if trimmedRaw == "" {
		return []string{}
	}

	tryNormalizePlainText := func(text string) []string {
		text = strings.TrimSpace(text)
		if text == "" {
			return []string{}
		}

		text = strings.Trim(text, "\"'")
		if text == "" {
			return []string{}
		}

		fields := strings.FieldsFunc(text, func(r rune) bool {
			switch r {
			case '\n', '\r', '\t', ',', ';':
				return true
			default:
				return false
			}
		})

		seen := map[string]struct{}{}
		normalized := make([]string, 0, len(fields))
		for _, field := range fields {
			field = strings.TrimSpace(strings.Trim(field, "\"'"))
			if field == "" {
				continue
			}
			if _, ok := seen[field]; ok {
				continue
			}
			seen[field] = struct{}{}
			normalized = append(normalized, field)
		}
		sort.Strings(normalized)
		return normalized
	}

	var values []string
	if err := json.Unmarshal([]byte(trimmedRaw), &values); err != nil {
		return tryNormalizePlainText(trimmedRaw)
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
		"template_code":   profile.TemplateCode,
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

func buildRoutingTemplateCopyIndex(profiles []models.RoutingProfile) map[string]models.RoutingProfile {
	index := map[string]models.RoutingProfile{}
	for _, profile := range profiles {
		if profile.Kind == models.RoutingProfileSystem {
			continue
		}
		templateCode := strings.TrimSpace(profile.TemplateCode)
		if templateCode == "" {
			continue
		}
		if _, exists := index[templateCode]; exists {
			continue
		}
		index[templateCode] = profile
	}
	return index
}

func normalizeRoutingProfileInput(values []string) []string {
	joined := mustMarshalStringArray(values)
	return decodeJSONStringArray(joined)
}
