package handlers

import (
	"encoding/json"
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

func parseConfigJSON(t *testing.T, raw string) map[string]interface{} {
	t.Helper()

	var parsed map[string]interface{}
	if err := json.Unmarshal([]byte(raw), &parsed); err != nil {
		t.Fatalf("unmarshal config: %v", err)
	}
	return parsed
}
