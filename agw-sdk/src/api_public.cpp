#include "agw/api.h"

#include "services/api_methods.h"
#include "util/json.h"

// Тонкий публичный слой: конвертирует std-структуры в внутренние services::* и обратно.
// nlohmann живёт только здесь и глубже, в include/agw/api.h его нет.
namespace agw::api {

namespace {

services::GatewayRequest toInternal(const GatewayRequest &r)
{
    services::GatewayRequest out;
    out.osVersion = r.osVersion;
    out.appVersion = r.appVersion;
    out.appLanguage = r.appLanguage;
    out.installationUuid = r.installationUuid;
    out.userCountryCode = r.userCountryCode;
    out.serverCountryCode = r.serverCountryCode;
    out.serviceType = r.serviceType;
    out.serviceProtocol = r.serviceProtocol;
    if (!r.authDataJson.empty()) {
        try {
            out.authData = util::Json::parse(r.authDataJson);
        } catch (...) {
        }
    }
    return out;
}

services::AccountRequest toAccount(const GatewayRequest &r, const std::string &cliVersion,
                                   const std::string &subscriptionStatus)
{
    services::AccountRequest out;
    out.base = toInternal(r);
    out.cliVersion = cliVersion;
    out.subscriptionStatus = subscriptionStatus;
    return out;
}

std::string dumpOrEmpty(const util::Json &j)
{
    return j.is_null() ? std::string() : j.dump();
}

ImportResult fromInternal(const services::ImportResult &r)
{
    ImportResult out;
    out.error = r.error;
    out.captchaRequired = r.captchaRequired;
    out.captcha.captchaId = r.captcha.captchaId;
    out.captcha.captchaImageBase64 = r.captcha.captchaImageBase64;
    out.captcha.hint = r.captcha.hint;
    out.serverConfigJson = r.serverConfigJson;
    out.rawResponseJson = r.rawResponseJson;
    return out;
}

} // namespace

JsonResult getServices(GatewayController &gw, const std::string &osVersion, const std::string &appVersion,
                       const std::string &cliName, const std::string &appLanguage)
{
    services::ServicesRequest req;
    req.osVersion = osVersion;
    req.appVersion = appVersion;
    req.cliName = cliName;
    req.appLanguage = appLanguage;
    const services::JsonResult r = services::getServices(gw, req);
    return {r.error, dumpOrEmpty(r.value)};
}

JsonResult getNews(GatewayController &gw, const std::string &locale,
                   const std::vector<std::string> &userCountryCodes, const std::vector<std::string> &serviceTypes)
{
    services::NewsRequest req;
    req.locale = locale;
    req.userCountryCodes = userCountryCodes;
    req.serviceTypes = serviceTypes;
    const services::JsonResult r = services::getNews(gw, req);
    return {r.error, dumpOrEmpty(r.value)};
}

UrlResult getUpdaterEndpoint(GatewayController &gw, const std::string &cliVersion, const std::string &osVersion,
                             const std::string &installationUuid)
{
    services::UpdaterRequest req;
    req.cliVersion = cliVersion;
    req.osVersion = osVersion;
    req.installationUuid = installationUuid;
    const services::UpdateResult r = services::getUpdaterEndpoint(gw, req);
    return {r.error, r.url, dumpOrEmpty(r.raw)};
}

AccountInfoResult getAccountInfo(GatewayController &gw, const GatewayRequest &req, const std::string &cliVersion,
                                 const std::string &subscriptionStatus)
{
    const services::AccountInfoResult r = services::getAccountInfo(gw, toAccount(req, cliVersion, subscriptionStatus));
    return {r.error, r.account};
}

RenewalResult getRenewalLink(GatewayController &gw, const GatewayRequest &req, const std::string &cliVersion,
                             const std::string &subscriptionStatus)
{
    const services::RenewalResult r = services::getRenewalLink(gw, toAccount(req, cliVersion, subscriptionStatus));
    return {r.error, r.renewalUrl};
}

ImportResult importService(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey)
{
    return fromInternal(services::importService(gw, toInternal(req), publicKey));
}

ImportResult importTrial(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                         const std::string &email)
{
    return fromInternal(services::importTrial(gw, toInternal(req), publicKey, email));
}

ImportResult resolveImportCaptcha(GatewayController &gw, const GatewayRequest &req, const std::string &publicKey,
                                  const std::string &captchaId, const std::string &captchaSolution)
{
    return fromInternal(services::resolveImportCaptcha(gw, toInternal(req), publicKey, captchaId, captchaSolution));
}

ErrorCode deactivateDevice(GatewayController &gw, const GatewayRequest &req)
{
    return services::deactivateDevice(gw, toInternal(req));
}

AppStoreImportResult importServiceFromAppStore(GatewayController &gw, const GatewayRequest &req,
                                               const std::string &publicKey, const std::string &transactionId)
{
    const services::AppStoreImportResult r =
            services::importServiceFromAppStore(gw, toInternal(req), publicKey, transactionId);
    return {r.error, r.serverConfigJson, r.vpnKey, r.crc};
}

} // namespace agw::api
