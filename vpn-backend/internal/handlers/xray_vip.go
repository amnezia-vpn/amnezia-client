package handlers

import (
	"encoding/json"
	"fmt"
	"regexp"
	"strconv"
	"strings"
	"vpn-backend/internal/models"

	"github.com/google/uuid"
	"gorm.io/gorm"
)

const (
	defaultXrayContainer = "amnezia-xray"
	legacyXrayContainer  = "fblink-xray"
	xrayProxyTag         = "proxy"
	xrayDirectTag        = "direct"
)

var xrayBasePaths = []string{
	"/opt/amnezia/xray",
	"/opt/fblink/xray",
}

type xrayRuntimeLocation struct {
	container  string
	configPath string
}

func candidateXrayContainers(preferred string) []string {
	candidates := make([]string, 0, 3)
	seen := map[string]struct{}{}

	appendCandidate := func(container string) {
		container = strings.TrimSpace(container)
		if container == "" {
			return
		}
		if _, ok := seen[container]; ok {
			return
		}
		seen[container] = struct{}{}
		candidates = append(candidates, container)
	}

	appendCandidate(defaultXrayContainer)
	appendCandidate(preferred)
	appendCandidate(legacyXrayContainer)

	return candidates
}

func xrayTemplateDefaults(template *models.VLESSServerTemplate, server *models.VPNServer) {
	if template.Address == "" {
		if server.Endpoint != "" {
			template.Address = strings.Split(server.Endpoint, ":")[0]
		}
		if template.Address == "" {
			template.Address = server.Host
		}
	}
	if template.Port == 0 {
		template.Port = 443
	}
	if template.Fingerprint == "" {
		template.Fingerprint = "chrome"
	}
	if template.Flow == "" {
		template.Flow = "xtls-rprx-vision"
	}
	if template.Network == "" {
		template.Network = "tcp"
	}
	if template.Security == "" {
		template.Security = "reality"
	}
	if template.ContainerName == "" {
		template.ContainerName = defaultXrayContainer
	}
}

func hasUsableVLESSTemplate(template *models.VLESSServerTemplate) bool {
	if template == nil {
		return false
	}

	return strings.TrimSpace(template.Address) != "" &&
		template.Port > 0 &&
		strings.TrimSpace(template.ServerName) != "" &&
		strings.TrimSpace(template.PublicKey) != "" &&
		strings.TrimSpace(template.ShortID) != ""
}

func resolveXrayRuntimeLocation(server *models.VPNServer, container string) xrayRuntimeLocation {
	for _, candidate := range candidateXrayContainers(container) {
		for _, basePath := range xrayBasePaths {
			path := basePath + "/server.json"
			cmd := fmt.Sprintf(`docker exec %s sh -c 'test -f %s && echo ok'`, candidate, path)
			if out, err := sshExec(server, cmd); err == nil && strings.Contains(out, "ok") {
				return xrayRuntimeLocation{container: candidate, configPath: path}
			}
		}
	}

	for _, basePath := range xrayBasePaths {
		path := basePath + "/server.json"
		hostCmd := fmt.Sprintf(`test -f %s && echo ok`, path)
		if out, err := sshExec(server, hostCmd); err == nil && strings.Contains(out, "ok") {
			return xrayRuntimeLocation{configPath: path}
		}
	}

	container = strings.TrimSpace(container)
	if container == "" {
		container = defaultXrayContainer
	}

	return xrayRuntimeLocation{
		container:  container,
		configPath: xrayBasePaths[0] + "/server.json",
	}
}

func detectXrayConfigPath(server *models.VPNServer, container string) string {
	return resolveXrayRuntimeLocation(server, container).configPath
}

func readXrayFile(server *models.VPNServer, container, path string) (string, error) {
	for _, candidate := range candidateXrayContainers(container) {
		cmd := fmt.Sprintf(`docker exec %s cat %s`, candidate, path)
		out, err := sshExec(server, cmd)
		if err == nil {
			return strings.TrimSpace(out), nil
		}
	}

	hostCmd := fmt.Sprintf(`cat %s`, path)
	out, hostErr := sshExec(server, hostCmd)
	if hostErr != nil {
		return "", fmt.Errorf("failed to read %s via docker or host", path)
	}
	return strings.TrimSpace(out), nil
}

func writeXrayFile(server *models.VPNServer, container, path, content string) error {
	escapedContent := strings.ReplaceAll(content, "\r", "")
	for _, candidate := range candidateXrayContainers(container) {
		cmd := fmt.Sprintf(`cat > /tmp/fblink_xray_server.json <<'EOF'
%s
EOF
docker cp /tmp/fblink_xray_server.json %s:%s
rm /tmp/fblink_xray_server.json
docker exec %s sh -lc "killall -KILL xray 2>/dev/null || true; nohup xray -config %s >/tmp/fblink_xray_runtime.log 2>&1 &"`, escapedContent, candidate, path, candidate, path)
		if _, err := sshExec(server, cmd); err == nil {
			return nil
		}
	}

	hostCmd := fmt.Sprintf(`cat > %s <<'EOF'
%s
EOF`, path, escapedContent)
	if _, err := sshExec(server, hostCmd); err != nil {
		return err
	}
	_, _ = sshExec(server, fmt.Sprintf(`sh -lc "killall -KILL xray 2>/dev/null || true; nohup xray -config %s >/tmp/fblink_xray_runtime.log 2>&1 &"`, path))
	return nil
}

func detectPublishedXrayPort(server *models.VPNServer, container string, internalPort int) int {
	if internalPort <= 0 {
		return 0
	}

	portPattern := regexp.MustCompile(`:(\d+)\s*$`)
	for _, candidate := range candidateXrayContainers(container) {
		cmd := fmt.Sprintf(`docker port %s %d/tcp`, candidate, internalPort)
		out, err := sshExec(server, cmd)
		if err != nil {
			continue
		}
		for _, line := range strings.Split(strings.TrimSpace(out), "\n") {
			line = strings.TrimSpace(line)
			if line == "" {
				continue
			}
			matches := portPattern.FindStringSubmatch(line)
			if len(matches) != 2 {
				continue
			}
			publishedPort, convErr := strconv.Atoi(matches[1])
			if convErr == nil && publishedPort > 0 {
				return publishedPort
			}
		}
	}

	return 0
}

func ensureHostTCPPortOpen(server *models.VPNServer, port int) {
	if port <= 0 {
		return
	}

	cmd := fmt.Sprintf(`sh -lc "
if command -v iptables >/dev/null 2>&1; then
  iptables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || iptables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if command -v ip6tables >/dev/null 2>&1; then
  ip6tables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || ip6tables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if command -v ufw >/dev/null 2>&1; then
  ufw allow %d/tcp >/dev/null 2>&1 || true
fi
if command -v firewall-cmd >/dev/null 2>&1; then
  firewall-cmd --zone=public --add-port=%d/tcp >/dev/null 2>&1 || true
  firewall-cmd --permanent --zone=public --add-port=%d/tcp >/dev/null 2>&1 || true
  firewall-cmd --reload >/dev/null 2>&1 || true
fi
"`, port, port, port, port, port, port, port)
	_, _ = sshExec(server, cmd)
}

func ensureXrayRuntimeReady(server *models.VPNServer, container, configPath string, port int) error {
	if port <= 0 {
		return nil
	}

	ensureHostTCPPortOpen(server, port)

	for _, candidate := range candidateXrayContainers(container) {
		cmd := fmt.Sprintf(`docker exec %s sh -lc "
if command -v iptables >/dev/null 2>&1; then
  iptables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || iptables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if command -v ip6tables >/dev/null 2>&1; then
  ip6tables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || ip6tables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if ! nc -z 127.0.0.1 %d >/dev/null 2>&1; then
  killall -KILL xray 2>/dev/null || true
  nohup xray -config %s >/tmp/fblink_xray_runtime.log 2>&1 &
  sleep 1
fi
nc -z 127.0.0.1 %d >/dev/null 2>&1 || (cat /tmp/fblink_xray_runtime.log 2>/dev/null || true; exit 1)
"`, candidate, port, port, port, port, port, configPath, port)
		if _, err := sshExec(server, cmd); err == nil {
			return nil
		}
	}

	hostCmd := fmt.Sprintf(`sh -lc "
if command -v iptables >/dev/null 2>&1; then
  iptables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || iptables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if command -v ip6tables >/dev/null 2>&1; then
  ip6tables -C INPUT -p tcp --dport %d -j ACCEPT 2>/dev/null || ip6tables -I INPUT 1 -p tcp --dport %d -j ACCEPT
fi
if ! nc -z 127.0.0.1 %d >/dev/null 2>&1; then
  killall -KILL xray 2>/dev/null || true
  nohup xray -config %s >/tmp/fblink_xray_runtime.log 2>&1 &
  sleep 1
fi
nc -z 127.0.0.1 %d >/dev/null 2>&1 || (cat /tmp/fblink_xray_runtime.log 2>/dev/null || true; exit 1)
"`, port, port, port, port, port, configPath, port)
	if _, err := sshExec(server, hostCmd); err != nil {
		return fmt.Errorf("xray runtime on %s:%d is not ready: %w", server.Name, port, err)
	}
	return nil
}

func fetchVLESSTemplateFromServer(server *models.VPNServer, existing *models.VLESSServerTemplate) (*models.VLESSServerTemplate, error) {
	container := defaultXrayContainer
	if existing != nil && existing.ContainerName != "" {
		container = existing.ContainerName
	}

	runtimeLocation := resolveXrayRuntimeLocation(server, container)
	if runtimeLocation.container != "" {
		container = runtimeLocation.container
	}
	configPath := runtimeLocation.configPath
	basePath := strings.TrimSuffix(configPath, "/server.json")

	serverConfigRaw, err := readXrayFile(server, container, configPath)
	if err != nil {
		return nil, err
	}
	publicKey, err := readXrayFile(server, container, basePath+"/xray_public.key")
	if err != nil {
		return nil, err
	}
	clientID, err := readXrayFile(server, container, basePath+"/xray_uuid.key")
	if err != nil {
		clientID = ""
	}
	shortID, err := readXrayFile(server, container, basePath+"/xray_short_id.key")
	if err != nil {
		return nil, err
	}

	var parsed map[string]interface{}
	if err := json.Unmarshal([]byte(serverConfigRaw), &parsed); err != nil {
		return nil, err
	}

	template := &models.VLESSServerTemplate{
		ClientID: strings.TrimSpace(clientID),
		Address: strings.TrimSpace(func() string {
			if existing != nil && strings.TrimSpace(existing.Address) != "" {
				return existing.Address
			}
			return server.Host
		}()),
		Port:          443,
		PublicKey:     strings.TrimSpace(publicKey),
		ShortID:       strings.TrimSpace(shortID),
		Fingerprint:   "chrome",
		Flow:          "xtls-rprx-vision",
		Network:       "tcp",
		Security:      "reality",
		SpiderX:       "",
		ContainerName: container,
	}

	if existing != nil {
		template.ServerID = existing.ServerID
		template.ID = existing.ID
		if strings.TrimSpace(existing.Fingerprint) != "" {
			template.Fingerprint = existing.Fingerprint
		}
		if strings.TrimSpace(existing.Flow) != "" {
			template.Flow = existing.Flow
		}
		if strings.TrimSpace(existing.SpiderX) != "" {
			template.SpiderX = existing.SpiderX
		}
	}

	if inbounds, ok := parsed["inbounds"].([]interface{}); ok && len(inbounds) > 0 {
		if inbound, ok := inbounds[0].(map[string]interface{}); ok {
			if port, ok := inbound["port"].(float64); ok && port > 0 {
				template.Port = int(port)
			}
			if streamSettings, ok := inbound["streamSettings"].(map[string]interface{}); ok {
				if network, ok := streamSettings["network"].(string); ok && network != "" {
					template.Network = network
				}
				if security, ok := streamSettings["security"].(string); ok && security != "" {
					template.Security = security
				}
				if realitySettings, ok := streamSettings["realitySettings"].(map[string]interface{}); ok {
					if serverNames, ok := realitySettings["serverNames"].([]interface{}); ok && len(serverNames) > 0 {
						if serverName, ok := serverNames[0].(string); ok {
							template.ServerName = serverName
						}
					}
				}
			}
		}
	}

	if publishedPort := detectPublishedXrayPort(server, container, template.Port); publishedPort > 0 {
		template.Port = publishedPort
	}

	xrayTemplateDefaults(template, server)
	if err := ensureXrayRuntimeReady(server, container, configPath, template.Port); err != nil {
		return nil, err
	}
	return template, nil
}

func ensureVLESSTemplate(db *gorm.DB, server *models.VPNServer) (*models.VLESSServerTemplate, error) {
	if server.VLESSTemplate != nil {
		xrayTemplateDefaults(server.VLESSTemplate, server)
	}

	if server.SSHPassword != "" {
		template, err := fetchVLESSTemplateFromServer(server, server.VLESSTemplate)
		if err == nil {
			template.ServerID = server.ID
			xrayTemplateDefaults(template, server)
			if err := db.Where("server_id = ?", server.ID).Assign(*template).FirstOrCreate(template).Error; err != nil {
				return nil, err
			}
			server.VLESSTemplate = template
			return template, nil
		}

		if hasUsableVLESSTemplate(server.VLESSTemplate) {
			fmt.Printf("[WARN] Falling back to cached VLESS template for server %s after SSH refresh failure: %v\n", server.Name, err)
			return server.VLESSTemplate, nil
		}

		return nil, err
	}

	if hasUsableVLESSTemplate(server.VLESSTemplate) {
		return server.VLESSTemplate, nil
	}

	return nil, nil
}

func ensureVLESSCredential(tx *gorm.DB, userID uint, server *models.VPNServer, template *models.VLESSServerTemplate) (*models.VLESSCredential, error) {
	var credential models.VLESSCredential
	if err := tx.Where("user_id = ? AND server_id = ?", userID, server.ID).First(&credential).Error; err == nil {
		if credential.RevokedAt == nil {
			if err := addXrayClient(server, template, credential.ClientID); err != nil {
				return nil, err
			}
			return &credential, nil
		}
		credential.RevokedAt = nil
		if err := tx.Save(&credential).Error; err != nil {
			return nil, err
		}
		if err := addXrayClient(server, template, credential.ClientID); err != nil {
			return nil, err
		}
		return &credential, nil
	}

	credential = models.VLESSCredential{
		UserID:   userID,
		ServerID: server.ID,
		ClientID: uuid.New().String(),
	}
	if err := addXrayClient(server, template, credential.ClientID); err != nil {
		return nil, err
	}
	if err := tx.Create(&credential).Error; err != nil {
		_ = removeXrayClient(server, template, credential.ClientID)
		return nil, err
	}
	return &credential, nil
}

func sanitizeInboundVLESSClients(clients []interface{}) ([]interface{}, bool) {
	sanitized := make([]interface{}, 0, len(clients))
	changed := false

	for _, item := range clients {
		client, ok := item.(map[string]interface{})
		if !ok {
			sanitized = append(sanitized, item)
			continue
		}

		if _, hasEncryption := client["encryption"]; hasEncryption {
			delete(client, "encryption")
			changed = true
		}
		sanitized = append(sanitized, client)
	}

	return sanitized, changed
}

func addXrayClient(server *models.VPNServer, template *models.VLESSServerTemplate, clientID string) error {
	if template != nil && strings.TrimSpace(template.ClientID) != "" {
		return nil
	}
	if server.SSHPassword == "" {
		return nil
	}

	container := template.ContainerName
	if container == "" {
		container = defaultXrayContainer
	}
	configPath := detectXrayConfigPath(server, container)
	serverConfigRaw, err := readXrayFile(server, container, configPath)
	if err != nil {
		return err
	}

	var parsed map[string]interface{}
	if err := json.Unmarshal([]byte(serverConfigRaw), &parsed); err != nil {
		return err
	}

	inbounds, ok := parsed["inbounds"].([]interface{})
	if !ok || len(inbounds) == 0 {
		return fmt.Errorf("xray server config missing inbounds")
	}
	inbound, ok := inbounds[0].(map[string]interface{})
	if !ok {
		return fmt.Errorf("invalid xray inbound format")
	}
	settings, ok := inbound["settings"].(map[string]interface{})
	if !ok {
		return fmt.Errorf("xray inbound missing settings")
	}
	clients, _ := settings["clients"].([]interface{})
	clients, changed := sanitizeInboundVLESSClients(clients)
	for _, item := range clients {
		if client, ok := item.(map[string]interface{}); ok && client["id"] == clientID {
			if changed {
				settings["clients"] = clients
				inbound["settings"] = settings
				inbounds[0] = inbound
				parsed["inbounds"] = inbounds

				updatedJSON, _ := json.Marshal(parsed)
				if err := writeXrayFile(server, container, configPath, string(updatedJSON)); err != nil {
					return err
				}
				return ensureXrayRuntimeReady(server, container, configPath, template.Port)
			}
			return nil
		}
	}

	clients = append(clients, map[string]interface{}{
		"id":   clientID,
		"flow": template.Flow,
	})
	settings["clients"] = clients
	inbound["settings"] = settings
	inbounds[0] = inbound
	parsed["inbounds"] = inbounds

	updatedJSON, _ := json.Marshal(parsed)
	if err := writeXrayFile(server, container, configPath, string(updatedJSON)); err != nil {
		return err
	}
	return ensureXrayRuntimeReady(server, container, configPath, template.Port)
}

func removeXrayClient(server *models.VPNServer, template *models.VLESSServerTemplate, clientID string) error {
	if template != nil && strings.TrimSpace(template.ClientID) != "" {
		return nil
	}
	if server.SSHPassword == "" || template == nil {
		return nil
	}

	container := template.ContainerName
	if container == "" {
		container = defaultXrayContainer
	}
	configPath := detectXrayConfigPath(server, container)
	serverConfigRaw, err := readXrayFile(server, container, configPath)
	if err != nil {
		return err
	}

	var parsed map[string]interface{}
	if err := json.Unmarshal([]byte(serverConfigRaw), &parsed); err != nil {
		return err
	}

	inbounds, ok := parsed["inbounds"].([]interface{})
	if !ok || len(inbounds) == 0 {
		return fmt.Errorf("xray server config missing inbounds")
	}
	inbound, ok := inbounds[0].(map[string]interface{})
	if !ok {
		return fmt.Errorf("invalid xray inbound format")
	}
	settings, ok := inbound["settings"].(map[string]interface{})
	if !ok {
		return fmt.Errorf("xray inbound missing settings")
	}
	clients, _ := settings["clients"].([]interface{})
	clients, _ = sanitizeInboundVLESSClients(clients)
	filtered := make([]interface{}, 0, len(clients))
	for _, item := range clients {
		client, ok := item.(map[string]interface{})
		if !ok || client["id"] != clientID {
			filtered = append(filtered, item)
		}
	}
	settings["clients"] = filtered
	inbound["settings"] = settings
	inbounds[0] = inbound
	parsed["inbounds"] = inbounds

	updatedJSON, _ := json.Marshal(parsed)
	if err := writeXrayFile(server, container, configPath, string(updatedJSON)); err != nil {
		return err
	}
	return ensureXrayRuntimeReady(server, container, configPath, template.Port)
}

func buildVIPRoutingRules(profiles []models.RoutingProfile) []map[string]interface{} {
	domainRulesByAction := map[models.RoutingProfileAction][]string{
		models.RoutingProfileDirect: {},
		models.RoutingProfileProxy:  {},
	}
	ipRulesByAction := map[models.RoutingProfileAction][]string{
		models.RoutingProfileDirect: {},
		models.RoutingProfileProxy:  {},
	}

	for _, profile := range profiles {
		if !profile.Enabled {
			continue
		}
		action := profile.Action
		if action != models.RoutingProfileProxy {
			action = models.RoutingProfileDirect
		}
		for _, domain := range decodeJSONStringArray(profile.DomainsJSON) {
			domainRulesByAction[action] = append(domainRulesByAction[action], "full:"+domain)
		}
		for _, suffix := range decodeJSONStringArray(profile.DomainSuffixesJSON) {
			trimmed := strings.TrimPrefix(strings.TrimSpace(suffix), ".")
			if trimmed == "" {
				continue
			}
			if strings.Contains(trimmed, ".") {
				domainRulesByAction[action] = append(domainRulesByAction[action], "domain:"+trimmed)
			} else {
				domainRulesByAction[action] = append(domainRulesByAction[action], "regexp:(^|\\.).*\\."+regexp.QuoteMeta(trimmed)+"$")
			}
		}
		ipRulesByAction[action] = append(ipRulesByAction[action], decodeJSONStringArray(profile.CIDRsJSON)...)
	}

	appendRules := func(rules []map[string]interface{}, action models.RoutingProfileAction, outboundTag string) []map[string]interface{} {
		domainRules := domainRulesByAction[action]
		ipRules := ipRulesByAction[action]
		if len(domainRules) > 0 {
			rules = append(rules, map[string]interface{}{
				"type":        "field",
				"outboundTag": outboundTag,
				"domain":      domainRules,
			})
		}
		if len(ipRules) > 0 {
			rules = append(rules, map[string]interface{}{
				"type":        "field",
				"outboundTag": outboundTag,
				"ip":          ipRules,
			})
		}
		return rules
	}

	rules := make([]map[string]interface{}, 0, 4)
	rules = appendRules(rules, models.RoutingProfileDirect, xrayDirectTag)
	rules = appendRules(rules, models.RoutingProfileProxy, xrayProxyTag)
	if len(rules) == 0 {
		return nil
	}
	return rules
}

func vipDNSProxyRules(dnsConfig vipDNSConfig) []map[string]interface{} {
	ips := make([]string, 0, 2)
	seen := map[string]struct{}{}

	appendIP := func(ip string) {
		ip = strings.TrimSpace(ip)
		if ip == "" {
			return
		}
		cidr := ip + "/32"
		if _, ok := seen[cidr]; ok {
			return
		}
		seen[cidr] = struct{}{}
		ips = append(ips, cidr)
	}

	appendIP(dnsConfig.Primary)
	appendIP(dnsConfig.Secondary)
	appendIP(vipDNSCleanIP)
	appendIP(vipDNSAdBlockIP)

	if len(ips) == 0 {
		return nil
	}

	return []map[string]interface{}{
		{
			"type":        "field",
			"outboundTag": xrayProxyTag,
			"ip":          ips,
		},
	}
}

func buildVLESSConfig(clientID string, server *models.VPNServer, template *models.VLESSServerTemplate, profiles []models.RoutingProfile, dnsConfig vipDNSConfig) map[string]interface{} {
	xrayTemplateDefaults(template, server)

	primaryOutbound := map[string]interface{}{
		"protocol": "vless",
		"settings": map[string]interface{}{
			"vnext": []interface{}{
				map[string]interface{}{
					"address": template.Address,
					"port":    template.Port,
					"users": []interface{}{
						map[string]interface{}{
							"id":         clientID,
							"flow":       template.Flow,
							"encryption": "none",
						},
					},
				},
			},
		},
		"streamSettings": map[string]interface{}{
			"network":  template.Network,
			"security": template.Security,
			"sockopt": map[string]interface{}{
				"tcpFastOpen":          true,
				"tcpKeepAliveIdle":     45,
				"tcpKeepAliveInterval": 45,
			},
			"realitySettings": map[string]interface{}{
				"fingerprint": template.Fingerprint,
				"serverName":  template.ServerName,
				"publicKey":   template.PublicKey,
				"shortId":     template.ShortID,
				"spiderX":     template.SpiderX,
			},
		},
	}

	xrayConfig := map[string]interface{}{
		"log": map[string]interface{}{
			"loglevel": "warning",
		},
		"inbounds": []interface{}{
			map[string]interface{}{
				"listen":   "127.0.0.1",
				"port":     10808,
				"protocol": "socks",
				"settings": map[string]interface{}{
					"auth": "noauth",
					"udp":  true,
				},
			},
		},
	}

	if routingRules := buildVIPRoutingRules(profiles); len(routingRules) > 0 {
		routingRules = append(vipDNSProxyRules(dnsConfig), routingRules...)
		inbounds := xrayConfig["inbounds"].([]interface{})
		inbound := inbounds[0].(map[string]interface{})
		inbound["sniffing"] = map[string]interface{}{
			"enabled":      true,
			"routeOnly":    false,
			"destOverride": []string{"http", "tls", "quic"},
		}
		inbounds[0] = inbound
		xrayConfig["inbounds"] = inbounds

		primaryOutbound["tag"] = xrayProxyTag
		xrayConfig["outbounds"] = []interface{}{
			primaryOutbound,
			map[string]interface{}{
				"tag":      xrayDirectTag,
				"protocol": "freedom",
			},
		}
		xrayConfig["routing"] = map[string]interface{}{
			"domainStrategy": "IPIfNonMatch",
			"rules":          routingRules,
		}
	} else {
		// Keep the generated config as close as possible to the native VLESS import shape
		// that the client already handles successfully.
		xrayConfig["outbounds"] = []interface{}{primaryOutbound}
	}

	lastConfigJSON, _ := json.Marshal(xrayConfig)
	container := map[string]interface{}{
		"container": "fblink-xray",
		"xray": map[string]interface{}{
			"last_config":        string(lastConfigJSON),
			"isThirdPartyConfig": true,
		},
	}

	description := "FBLink VPN - " + server.Region
	if strings.TrimSpace(server.Region) == "" {
		description = "FBLink VPN - " + server.Name
	}

	return map[string]interface{}{
		"containers":       []interface{}{container},
		"defaultContainer": "fblink-xray",
		"description":      description,
		"country_code":     server.CountryCode,
		"dns1":             dnsConfig.Primary,
		"dns2":             dnsConfig.Secondary,
		"hostName":         template.Address,
	}
}
