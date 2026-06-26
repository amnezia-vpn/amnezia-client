#ifndef AGW_SERVICES_VPN_KEY_H
#define AGW_SERVICES_VPN_KEY_H

#include <string>

#include "util/json.h"

namespace agw::services {

// Сборка vpn://-ключа из конфига сервера, байт-в-байт с apiUtils::getPremiumV1/V2VpnKey:
//   escapeUnicode(вручную собранный JSON) → qCompress(.,6) без 4-байтового префикса →
//   сигнатура 00 00 00 ff + поток → base64url(с паддингом) → "vpn://" + результат.
// Вход — серверный конфиг (как в ответе шлюза). Тип конфига проверяет вызывающий.
std::string buildPremiumV1VpnKey(const util::Json &serverConfig);
std::string buildPremiumV2VpnKey(const util::Json &serverConfig);

} // namespace agw::services

#endif // AGW_SERVICES_VPN_KEY_H
