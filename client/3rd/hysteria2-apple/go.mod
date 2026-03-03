module github.com/amnezia-vpn/hysteria2-apple

go 1.21

require (
	github.com/apernet/hysteria/app/v2 v2.0.0
	github.com/apernet/hysteria/core/v2 v2.0.0
	github.com/apernet/hysteria/extras/v2 v2.0.0
	gopkg.in/yaml.v3 v3.0.1
)

replace (
	github.com/apernet/hysteria/app/v2 => github.com/apernet/hysteria/app/v2 v2.6.1
	github.com/apernet/hysteria/core/v2 => github.com/apernet/hysteria/core/v2 v2.6.1
	github.com/apernet/hysteria/extras/v2 => github.com/apernet/hysteria/extras/v2 v2.6.1
)
