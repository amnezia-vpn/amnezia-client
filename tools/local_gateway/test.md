В README мока уже есть curl. Логика такая:

Терминал A — как «TV», долгий запрос:

curl -i -N -X POST "http://127.0.0.1:8080/api/v1/generate_qr" \
-H "Content-Type: application/json" \
-d '{"qr_uuid":"123e4567-e89b-12d3-a456-426614174000","installation_uuid":"tv-install","app_version":"1.0","os_version":"test"}'

Терминал B — как «телефон», пока A висит:

curl -i -X POST "http://127.0.0.1:8080/api/v1/scan_qr" \
-H "Content-Type: application/json" \
-d '{
"qr_uuid":"123e4567-e89b-12d3-a456-426614174000",
"config":"vpn://test",
"service_info":{"is_ad_visible":false},
"supported_protocols":["awg"],
"auth_data":{"api_key":"valid-local-key"},
"installation_uuid":"phone-install",
"app_version":"1.0",
"os_version":"test"
}'

Ожидание: в B — 200 с {"message":"OK"}, в A — 200 с полями config / service_info / supported_protocols.
Так вы убеждаетесь, что мок и сценарий pairing живые.
