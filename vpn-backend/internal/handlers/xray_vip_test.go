package handlers

import (
	"encoding/json"
	"fmt"
	"testing"
	"vpn-backend/internal/models"
)

func TestBuildVLESSConfigPinsVIPDNSOverProxy(t *testing.T) {
	server := &models.VPNServer{Name: "Test", Region: "AMS", CountryCode: "NL"}
	template := &models.VLESSServerTemplate{
		Address:     "138.124.101.69",
		Port:        8443,
		ServerName:  "www.googletagmanager.com",
		PublicKey:   "pub",
		ShortID:     "short",
		Fingerprint: "chrome",
		Flow:        "xtls-rprx-vision",
		Network:     "tcp",
		Security:    "reality",
	}
	profiles := []models.RoutingProfile{
		{
			Enabled:            true,
			Action:             models.RoutingProfileProxy,
			DomainSuffixesJSON: `["youtube.com"]`,
		},
	}

	config := buildVLESSConfig("client-id", server, template, profiles, vipDNSConfig{Primary: vipDNSAdBlockIP})
	xrayConfig := config["containers"].([]interface{})[0].(map[string]interface{})["xray"].(map[string]interface{})["last_config"].(string)
	if xrayConfig == "" {
		t.Fatalf("expected serialized xray config")
	}

	parsed := parseConfigJSON(t, xrayConfig)
	rules := parsed["routing"].(map[string]interface{})["rules"].([]interface{})
	if len(rules) == 0 {
		t.Fatalf("expected routing rules")
	}

	firstRule := rules[0].(map[string]interface{})
	if firstRule["outboundTag"] != xrayProxyTag {
		t.Fatalf("expected first rule outboundTag=%q, got %v", xrayProxyTag, firstRule["outboundTag"])
	}

	ipRules := firstRule["ip"].([]interface{})
	if len(ipRules) < 2 {
		t.Fatalf("expected vip dns proxy rule to contain both internal dns IPs, got %v", ipRules)
	}
}

func TestBuildVLESSConfigAcceptsLegacyPlainTextRules(t *testing.T) {
	server := &models.VPNServer{Name: "Test", Region: "AMS", CountryCode: "NL"}
	template := &models.VLESSServerTemplate{
		Address:     "138.124.101.69",
		Port:        8443,
		ServerName:  "www.googletagmanager.com",
		PublicKey:   "pub",
		ShortID:     "short",
		Fingerprint: "chrome",
		Flow:        "xtls-rprx-vision",
		Network:     "tcp",
		Security:    "reality",
	}
	profiles := []models.RoutingProfile{
		{
			Enabled:            true,
			Action:             models.RoutingProfileProxy,
			DomainsJSON:        "chatgpt.com\nclaude.ai",
			DomainSuffixesJSON: ".openai.com, .anthropic.com",
		},
	}

	config := buildVLESSConfig("client-id", server, template, profiles, vipDNSConfig{Primary: vipDNSCleanIP})
	xrayConfig := config["containers"].([]interface{})[0].(map[string]interface{})["xray"].(map[string]interface{})["last_config"].(string)
	if xrayConfig == "" {
		t.Fatalf("expected serialized xray config")
	}

	parsed := parseConfigJSON(t, xrayConfig)
	rules := parsed["routing"].(map[string]interface{})["rules"].([]interface{})
	if len(rules) == 0 {
		t.Fatalf("expected routing rules for legacy plain-text profile fields")
	}

	hasAIProxyRule := false
	for _, ruleValue := range rules {
		rule := ruleValue.(map[string]interface{})
		if rule["outboundTag"] != xrayProxyTag {
			continue
		}
		domains, ok := rule["domain"].([]interface{})
		if !ok || len(domains) == 0 {
			continue
		}
		if containsInterfaceString(domains, "full:chatgpt.com") &&
			containsInterfaceString(domains, "full:claude.ai") &&
			containsInterfaceString(domains, "domain:openai.com") &&
			containsInterfaceString(domains, "domain:anthropic.com") {
			hasAIProxyRule = true
			break
		}
	}

	if !hasAIProxyRule {
		t.Fatalf("expected proxy routing rule with AI domains from legacy plain text fields, got rules=%v", rules)
	}
}

func TestBuildVLESSConfigIgnoresSystemTemplateProfiles(t *testing.T) {
	server := &models.VPNServer{Name: "Test", Region: "AMS", CountryCode: "NL"}
	template := &models.VLESSServerTemplate{
		Address:     "138.124.101.69",
		Port:        8443,
		ServerName:  "www.googletagmanager.com",
		PublicKey:   "pub",
		ShortID:     "short",
		Fingerprint: "chrome",
		Flow:        "xtls-rprx-vision",
		Network:     "tcp",
		Security:    "reality",
	}
	profiles := []models.RoutingProfile{
		{
			Kind:               models.RoutingProfileSystem,
			Enabled:            true,
			Action:             models.RoutingProfileProxy,
			DomainSuffixesJSON: `["youtube.com"]`,
		},
		{
			Kind:               models.RoutingProfileCustom,
			Enabled:            true,
			Action:             models.RoutingProfileProxy,
			DomainSuffixesJSON: `["chatgpt.com"]`,
		},
	}

	config := buildVLESSConfig("client-id", server, template, profiles, vipDNSConfig{Primary: vipDNSCleanIP})
	xrayConfig := config["containers"].([]interface{})[0].(map[string]interface{})["xray"].(map[string]interface{})["last_config"].(string)
	parsed := parseConfigJSON(t, xrayConfig)
	rules := parsed["routing"].(map[string]interface{})["rules"].([]interface{})

	containsYouTubeSystemDomain := false
	containsChatGPTCustomDomain := false
	for _, ruleValue := range rules {
		rule := ruleValue.(map[string]interface{})
		domains, ok := rule["domain"].([]interface{})
		if !ok {
			continue
		}
		if containsInterfaceString(domains, "domain:youtube.com") {
			containsYouTubeSystemDomain = true
		}
		if containsInterfaceString(domains, "domain:chatgpt.com") {
			containsChatGPTCustomDomain = true
		}
	}

	if containsYouTubeSystemDomain {
		t.Fatalf("system template routing rules must not be applied at runtime, got rules=%v", rules)
	}
	if !containsChatGPTCustomDomain {
		t.Fatalf("custom routing rules must still be applied, got rules=%v", rules)
	}
}

func TestBuildVLESSConfigUsesSystemProfilesFallbackWhenNoCustomEnabled(t *testing.T) {
	server := &models.VPNServer{Name: "Test", Region: "AMS", CountryCode: "NL"}
	template := &models.VLESSServerTemplate{
		Address:     "138.124.101.69",
		Port:        8443,
		ServerName:  "www.googletagmanager.com",
		PublicKey:   "pub",
		ShortID:     "short",
		Fingerprint: "chrome",
		Flow:        "xtls-rprx-vision",
		Network:     "tcp",
		Security:    "reality",
	}
	profiles := []models.RoutingProfile{
		{
			Kind:               models.RoutingProfileSystem,
			Enabled:            true,
			Action:             models.RoutingProfileProxy,
			DomainSuffixesJSON: `["youtube.com"]`,
		},
	}

	config := buildVLESSConfig("client-id", server, template, profiles, vipDNSConfig{Primary: vipDNSCleanIP})
	xrayConfig := config["containers"].([]interface{})[0].(map[string]interface{})["xray"].(map[string]interface{})["last_config"].(string)
	parsed := parseConfigJSON(t, xrayConfig)
	rules := parsed["routing"].(map[string]interface{})["rules"].([]interface{})

	containsYouTubeSystemDomain := false
	for _, ruleValue := range rules {
		rule := ruleValue.(map[string]interface{})
		domains, ok := rule["domain"].([]interface{})
		if !ok {
			continue
		}
		if containsInterfaceString(domains, "domain:youtube.com") {
			containsYouTubeSystemDomain = true
			break
		}
	}

	if !containsYouTubeSystemDomain {
		t.Fatalf("expected system profile rules to be used as fallback when no custom profile is enabled, got rules=%v", rules)
	}
}

func parseConfigJSON(t *testing.T, raw string) map[string]interface{} {
	t.Helper()

	var parsed map[string]interface{}
	if err := json.Unmarshal([]byte(raw), &parsed); err != nil {
		t.Fatalf("unmarshal config: %v", err)
	}
	return parsed
}

func containsInterfaceString(values []interface{}, needle string) bool {
	for _, value := range values {
		if fmt.Sprint(value) == needle {
			return true
		}
	}
	return false
}
