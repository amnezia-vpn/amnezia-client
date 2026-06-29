#include "services/api_methods.h"

#include <vector>

#include "protocol/keys.h"
#include "services/models.h"
#include "util/base64.h"
#include "util/checksum.h"
#include "util/json.h"
#include "util/zlib.h"

namespace agw::services {

namespace {

namespace k = protocol::keys;

// Парсинг тела ответа; при сбое — null.
util::Json parseBody(const std::string &body)
{
    try {
        return util::Json::parse(body);
    } catch (...) {
        return util::Json();
    }
}

util::Json toArray(const std::vector<std::string> &items)
{
    util::Json arr = util::Json::array();
    for (const auto &s : items) {
        arr.push_back(s);
    }
    return arr;
}

} // namespace

JsonResult getServices(GatewayController &gw, const ServicesRequest &req)
{
    util::Json payload;
    payload[k::osVersion] = req.osVersion;
    payload[k::appVersion] = req.appVersion;
    payload[k::cliName] = req.cliName;
    payload[k::appLanguage] = req.appLanguage;

    const Response r = gw.post("%1v1/services", util::qtIndentedDump(payload), FailoverContext{});
    if (r.error != ErrorCode::NoError) {
        return {r.error, util::Json()};
    }

    util::Json doc = parseBody(r.body);
    if (!doc.is_object() || !doc.contains(k::services)) {
        return {ErrorCode::ApiServicesMissingError, util::Json()};
    }
    return {ErrorCode::NoError, std::move(doc)};
}

JsonResult getNews(GatewayController &gw, const NewsRequest &req)
{
    util::Json payload;
    payload[k::locale] = req.locale;
    if (!req.userCountryCodes.empty()) {
        payload[k::userCountryCode] = toArray(req.userCountryCodes);
    }
    if (!req.serviceTypes.empty()) {
        payload[k::serviceType] = toArray(req.serviceTypes);
    }

    const Response r = gw.post("%1v1/news", util::qtIndentedDump(payload), FailoverContext{});
    if (r.error != ErrorCode::NoError) {
        return {r.error, util::Json()};
    }

    util::Json doc = parseBody(r.body);
    if (doc.is_array()) {
        return {ErrorCode::NoError, std::move(doc)};
    }
    if (doc.is_object() && doc.contains(k::news) && doc[k::news].is_array()) {
        return {ErrorCode::NoError, doc[k::news]};
    }
    return {ErrorCode::NoError, util::Json::array()};
}

UpdateResult getUpdaterEndpoint(GatewayController &gw, const UpdaterRequest &req)
{
    util::Json payload;
    payload[k::cliVersion] = req.cliVersion;
    payload[k::osVersion] = req.osVersion;
    payload[k::installationUuid] = req.installationUuid;

    const Response r = gw.post("%1v1/updater_endpoint", util::qtIndentedDump(payload), FailoverContext{});
    if (r.error != ErrorCode::NoError) {
        return {r.error, std::string(), util::Json()};
    }

    util::Json doc = parseBody(r.body);
    std::string url;
    if (doc.is_object() && doc.contains(k::url) && doc[k::url].is_string()) {
        url = doc[k::url].get<std::string>();
        if (!url.empty() && url.back() == '/') {
            url.pop_back();  // chop trailing '/'
        }
    }
    return {ErrorCode::NoError, std::move(url), std::move(doc)};
}

namespace {

// Паритет с GatewayRequestData::toJsonObject — кладёт только непустые поля.
util::Json gatewayRequestToJson(const GatewayRequest &r)
{
    util::Json p = util::Json::object();
    auto setIf = [&](const char *key, const std::string &v) {
        if (!v.empty()) p[key] = v;
    };
    setIf(k::osVersion, r.osVersion);
    setIf(k::appVersion, r.appVersion);
    setIf(k::appLanguage, r.appLanguage);
    setIf(k::installationUuid, r.installationUuid);
    setIf(k::userCountryCode, r.userCountryCode);
    setIf(k::serverCountryCode, r.serverCountryCode);
    setIf(k::serviceType, r.serviceType);
    setIf(k::serviceProtocol, r.serviceProtocol);
    if (r.authData.is_object() && !r.authData.empty()) {
        p[k::authData] = r.authData;
    }
    return p;
}

util::Json buildAccountPayload(const AccountRequest &r)
{
    util::Json p = gatewayRequestToJson(r.base);
    // cli_version и subscription_status кладутся всегда (как в оригинале).
    p[k::cliVersion] = r.cliVersion;
    p[k::subscriptionStatus] = r.subscriptionStatus;
    return p;
}

// appendProtocolDataToApiPayload: awg/vless → public_key.
void appendPublicKey(util::Json &payload, const std::string &protocol, const std::string &publicKey)
{
    if (protocol == k::protoAwg || protocol == k::protoVless) {
        payload[k::publicKey] = publicKey;
    }
}

// Нормализация решения капчи (паритет): оставляем ASCII-цифры, полноширинные 0xFF10..0xFF19 → ASCII;
// если пусто — обрезанный исходник.
std::string normalizeCaptchaSolution(const std::string &s)
{
    std::string out;
    std::size_t i = 0;
    const std::size_t n = s.size();
    while (i < n) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 0x80) {
            if (c >= '0' && c <= '9') out.push_back(static_cast<char>(c));
            i += 1;
        } else if ((c >> 5) == 0x6 && i + 1 < n) {
            i += 2;  // 2-байтовый UTF-8, не цифра
        } else if ((c >> 4) == 0xE && i + 2 < n) {
            const std::uint32_t cp = (c & 0x0F) << 12 | (static_cast<unsigned char>(s[i + 1]) & 0x3F) << 6
                                   | (static_cast<unsigned char>(s[i + 2]) & 0x3F);
            if (cp >= 0xFF10 && cp <= 0xFF19) {
                out.push_back(static_cast<char>('0' + (cp - 0xFF10)));
            }
            i += 3;
        } else {
            i += 1;
        }
    }
    if (!out.empty()) {
        return out;
    }
    // trim исходника
    std::size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\n' || s[b] == '\r')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\n' || s[e - 1] == '\r')) --e;
    return s.substr(b, e - b);
}

bool parseCaptcha(const util::Json &doc, CaptchaInfo &out)
{
    if (!doc.is_object() || !doc.contains(k::captchaId) || !doc.contains(k::captchaImage)) {
        return false;
    }
    auto str = [&](const char *key) {
        auto it = doc.find(key);
        return (it != doc.end() && it->is_string()) ? it->get<std::string>() : std::string();
    };
    out.captchaId = str(k::captchaId);
    out.captchaImageBase64 = str(k::captchaImage);
    out.hint = str(k::hint);
    return true;
}

// strip "vpn://" + base64url-decode → normalizedKey + сырые байты ключа.
std::string normalizeVpnKey(const std::string &raw)
{
    const std::string prefix = "vpn://";
    if (raw.rfind(prefix, 0) == 0) {
        return raw.substr(prefix.size());
    }
    return raw;
}

// distillConfig: normalizedKey → base64url-decode → qUncompress (если не сжато — как есть).
// Плейсхолдер $WIREGUARD_CLIENT_PRIVATE_KEY НЕ трогаем.
ErrorCode distillConfig(const std::string &normalizedKey, std::string &outConfig)
{
    const std::vector<std::uint8_t> ba = util::base64Decode(normalizedKey);
    if (ba.empty()) {
        return ErrorCode::ApiConfigEmptyError;
    }
    try {
        const std::vector<std::uint8_t> un = util::qtUncompress(ba);
        outConfig.assign(un.begin(), un.end());
    } catch (...) {
        outConfig.assign(ba.begin(), ba.end());  // не сжато — как есть
    }
    return ErrorCode::NoError;
}

// extractServerConfigJsonFromResponse (сетевая часть): field → strip "vpn://" → base64url-decode → qUncompress.
ErrorCode extractConfig(const util::Json &response, const char *field, std::string &outConfig)
{
    std::string data;
    if (response.is_object()) {
        auto it = response.find(field);
        if (it != response.end() && it->is_string()) {
            data = it->get<std::string>();
        }
    }
    return distillConfig(normalizeVpnKey(data), outConfig);
}

ImportResult importCommon(GatewayController &gw, const std::string &endpoint, util::Json payload,
                          const GatewayRequest &req)
{
    const Response r = gw.post(endpoint, util::qtIndentedDump(payload),
                               FailoverContext{req.serviceType, req.userCountryCode});

    ImportResult out;
    out.rawResponseJson = r.body;  // сырое тело — приложению для service_info/supported_protocols
    util::Json doc = parseBody(r.body);

    // капча: при Required (и при Invalid/Refresh для retry) тащим из тела
    if (r.error == ErrorCode::ApiCaptchaRequiredError || r.error == ErrorCode::ApiCaptchaInvalidError
        || r.error == ErrorCode::ApiCaptchaRefreshError) {
        out.error = r.error;
        out.captchaRequired = parseCaptcha(doc, out.captcha);
        return out;
    }
    if (r.error != ErrorCode::NoError) {
        out.error = r.error;
        return out;
    }
    out.error = extractConfig(doc, k::config, out.serverConfigJson);
    return out;
}

// ConfigSource::AmneziaGateway — config_version валидного gateway-конфига (Telegram=1, Gateway=2).
constexpr int kConfigSourceAmneziaGateway = 2;

} // namespace

AccountInfoResult getAccountInfo(GatewayController &gw, const AccountRequest &req)
{
    const util::Json payload = buildAccountPayload(req);
    const Response r = gw.post("%1v1/account_info", util::qtIndentedDump(payload),
                               FailoverContext{req.base.serviceType, req.base.userCountryCode});
    if (r.error != ErrorCode::NoError) {
        return {r.error, ApiConfig{}};
    }
    return {ErrorCode::NoError, parseApiConfig(parseBody(r.body))};
}

RenewalResult getRenewalLink(GatewayController &gw, const AccountRequest &req)
{
    const util::Json payload = buildAccountPayload(req);
    const Response r = gw.post("%1v1/renewal_link", util::qtIndentedDump(payload),
                               FailoverContext{req.base.serviceType, req.base.userCountryCode});
    if (r.error != ErrorCode::NoError) {
        return {r.error, std::string()};
    }
    util::Json doc = parseBody(r.body);
    std::string urlStr;
    if (doc.is_object() && doc.contains(k::renewalUrl) && doc[k::renewalUrl].is_string()) {
        urlStr = doc[k::renewalUrl].get<std::string>();
    }
    return {ErrorCode::NoError, std::move(urlStr)};
}

ImportResult importService(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey)
{
    util::Json payload = gatewayRequestToJson(req);
    appendPublicKey(payload, req.serviceProtocol, publicKey);
    return importCommon(gw, "%1v1/config", std::move(payload), req);
}

ImportResult importTrial(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                         const std::string &email)
{
    util::Json payload = gatewayRequestToJson(req);
    appendPublicKey(payload, req.serviceProtocol, publicKey);
    payload[k::email] = email;
    return importCommon(gw, "%1v1/trial", std::move(payload), req);
}

ImportResult resolveImportCaptcha(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                                  const std::string &captchaId, const std::string &captchaSolution)
{
    util::Json payload = gatewayRequestToJson(req);
    appendPublicKey(payload, req.serviceProtocol, publicKey);
    payload[k::captchaId] = captchaId;
    payload[k::captchaSolution] = normalizeCaptchaSolution(captchaSolution);
    return importCommon(gw, "%1v1/config", std::move(payload), req);
}

ErrorCode deactivateDevice(GatewayController &gw, const GatewayRequest &req)
{
    const util::Json payload = gatewayRequestToJson(req);
    const Response r = gw.post("%1v1/revoke_config", util::qtIndentedDump(payload),
                               FailoverContext{req.serviceType, req.userCountryCode});
    return r.error;
}

AppStoreImportResult importServiceFromAppStore(GatewayController &gw, const GatewayRequest &req,
                                               const std::string &publicKey, const std::string &transactionId)
{
    util::Json payload = gatewayRequestToJson(req);
    appendPublicKey(payload, req.serviceProtocol, publicKey);
    payload[k::transactionId] = transactionId;

    const Response r = gw.post("%1v1/subscriptions", util::qtIndentedDump(payload),
                               FailoverContext{req.serviceType, req.userCountryCode});

    AppStoreImportResult out;
    if (r.error != ErrorCode::NoError) {
        out.error = r.error;
        return out;
    }

    util::Json doc = parseBody(r.body);
    std::string rawKey;
    if (doc.is_object()) {
        auto it = doc.find(k::key);
        if (it != doc.end() && it->is_string()) {
            rawKey = it->get<std::string>();
        }
    }
    if (rawKey.empty()) {
        out.error = ErrorCode::ApiPurchaseError;
        return out;
    }

    out.vpnKey = normalizeVpnKey(rawKey);
    const ErrorCode ec = distillConfig(out.vpnKey, out.serverConfigJson);
    if (ec != ErrorCode::NoError || out.serverConfigJson.empty()) {
        out.error = ErrorCode::ApiPurchaseError;
        return out;
    }

    util::Json cfg = parseBody(out.serverConfigJson);
    if (!cfg.is_object() || cfg.value(k::configVersion, 0) != kConfigSourceAmneziaGateway) {
        out.error = ErrorCode::ApiPurchaseError;  // оригинал: InternalError (в SDK нет — IAP-сбой)
        return out;
    }

    // crc = qChecksum(QJsonDocument(configObject).toJson()) — над Qt-indented JSON, не над сырыми байтами.
    out.crc = util::qtChecksum16(util::qtIndentedDump(cfg));
    out.error = ErrorCode::NoError;
    return out;
}

} // namespace agw::services
