#include "services/vpn_key.h"

#include <cstdio>
#include <cstdint>
#include <vector>

#include "protocol/keys.h"
#include "util/base64.h"
#include "util/zlib.h"

namespace agw::services {

namespace {

namespace k = protocol::keys;

// Сигнатура AmneziaVPN-ключа: QByteArray::fromHex("000000ff").
const std::vector<std::uint8_t> kSignature = {0x00, 0x00, 0x00, 0xFF};

// Паритет с apiUtils::escapeUnicode (Qt-версия итерирует UTF-16-юниты QString):
// декодируем UTF-8 → код-поинты → UTF-16-юниты; символы < 0x20 или > 0x7E → \uXXXX, иначе как есть.
std::string escapeUnicode(const std::string &utf8)
{
    auto emitUnit = [](std::string &out, unsigned u) {
        if (u < 0x20 || u > 0x7E) {
            static const char *hex = "0123456789abcdef";
            out += "\\u";
            out.push_back(hex[(u >> 12) & 0xF]);
            out.push_back(hex[(u >> 8) & 0xF]);
            out.push_back(hex[(u >> 4) & 0xF]);
            out.push_back(hex[u & 0xF]);
        } else {
            out.push_back(static_cast<char>(u));
        }
    };

    std::string out;
    std::size_t i = 0;
    const std::size_t n = utf8.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        std::uint32_t cp = 0;
        std::size_t len = 1;
        if (c < 0x80) {
            cp = c;
        } else if ((c >> 5) == 0x6 && i + 1 < n) {
            cp = (c & 0x1F) << 6 | (static_cast<unsigned char>(utf8[i + 1]) & 0x3F);
            len = 2;
        } else if ((c >> 4) == 0xE && i + 2 < n) {
            cp = (c & 0x0F) << 12 | (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 6
               | (static_cast<unsigned char>(utf8[i + 2]) & 0x3F);
            len = 3;
        } else if ((c >> 3) == 0x1E && i + 3 < n) {
            cp = (c & 0x07) << 18 | (static_cast<unsigned char>(utf8[i + 1]) & 0x3F) << 12
               | (static_cast<unsigned char>(utf8[i + 2]) & 0x3F) << 6
               | (static_cast<unsigned char>(utf8[i + 3]) & 0x3F);
            len = 4;
        } else {
            cp = c; // некорректный байт — как есть
        }
        i += len;

        if (cp <= 0xFFFF) {
            emitUnit(out, cp);
        } else {
            // суррогатная пара UTF-16
            const std::uint32_t v = cp - 0x10000;
            emitUnit(out, 0xD800 + (v >> 10));
            emitUnit(out, 0xDC00 + (v & 0x3FF));
        }
    }
    return out;
}

std::string getStr(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

double getNum(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_number()) ? it->get<double>() : 0.0;
}

std::string finalize(const std::string &vpnKeyStr)
{
    const std::string escaped = escapeUnicode(vpnKeyStr);
    const std::vector<std::uint8_t> raw(escaped.begin(), escaped.end());

    std::vector<std::uint8_t> compressed = util::qtCompress(raw, 6);
    compressed.erase(compressed.begin(), compressed.begin() + 4); // .mid(4) — снять Qt-префикс

    std::vector<std::uint8_t> signed_;
    signed_.reserve(kSignature.size() + compressed.size());
    signed_.insert(signed_.end(), kSignature.begin(), kSignature.end());
    signed_.insert(signed_.end(), compressed.begin(), compressed.end());

    return "vpn://" + util::base64UrlEncode(signed_);
}

} // namespace

std::string buildPremiumV1VpnKey(const util::Json &serverConfig)
{
    char ver[32];
    std::snprintf(ver, sizeof(ver), "%.1f", getNum(serverConfig, k::configVersion));

    std::string s = "{";
    s += "\"" + std::string(k::name) + "\": \"" + getStr(serverConfig, k::name) + "\"";
    s += ", \"" + std::string(k::description) + "\": \"" + getStr(serverConfig, k::description) + "\"";
    s += ", \"" + std::string(k::configVersion) + "\": " + ver;
    s += ", \"" + std::string(k::protocol) + "\": \"" + getStr(serverConfig, k::protocol) + "\"";
    s += ", \"" + std::string(k::apiEndpoint) + "\": \"" + getStr(serverConfig, k::apiEndpoint) + "\"";
    s += ", \"" + std::string(k::apiKey) + "\": \"" + getStr(serverConfig, k::apiKey) + "\"";
    s += "}";
    return finalize(s);
}

std::string buildPremiumV2VpnKey(const util::Json &serverConfig)
{
    util::Json apiConfig = serverConfig.contains(k::apiConfig) ? serverConfig.at(k::apiConfig) : util::Json::object();
    util::Json authData = serverConfig.contains(k::authData) ? serverConfig.at(k::authData) : util::Json::object();

    const std::string name = getStr(serverConfig, k::name);
    const std::string description = getStr(serverConfig, k::description);
    const int configVersion = static_cast<int>(getNum(serverConfig, k::configVersion));
    const std::string serviceType = getStr(apiConfig, k::serviceType);
    const std::string serviceProtocol = getStr(apiConfig, k::serviceProtocol);
    const std::string userCountryCode = getStr(apiConfig, k::userCountryCode);
    const std::string apiKey = getStr(authData, k::apiKey);

    std::string s = "{";
    s += "\"" + std::string(k::name) + "\": \"" + name + "\", ";
    s += "\"" + std::string(k::description) + "\": \"" + description + "\", ";
    s += "\"" + std::string(k::configVersion) + "\": " + std::to_string(configVersion) + ", ";
    s += "\"" + std::string(k::apiConfig) + "\": {";
    s += "\"" + std::string(k::serviceType) + "\": \"" + serviceType + "\", ";
    s += "\"service_protocol\": \"" + serviceProtocol + "\", ";
    s += "\"user_country_code\": \"" + userCountryCode + "\"";
    s += "}, ";
    s += "\"auth_data\": {";
    s += "\"" + std::string(k::apiKey) + "\": \"" + apiKey + "\"";
    s += "}";
    s += "}";
    return finalize(s);
}

} // namespace agw::services
