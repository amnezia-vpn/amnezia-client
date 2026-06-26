#include "services/api_methods.h"

#include "protocol/keys.h"
#include "services/models.h"
#include "util/json.h"

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

// Паритет с GatewayRequestData::toJsonObject + добавление cli_version/subscription_status.
util::Json buildAccountPayload(const AccountRequest &r)
{
    util::Json p = util::Json::object();
    auto setIf = [&](const char *key, const std::string &v) {
        if (!v.empty()) p[key] = v;
    };
    setIf(k::osVersion, r.base.osVersion);
    setIf(k::appVersion, r.base.appVersion);
    setIf(k::appLanguage, r.base.appLanguage);
    setIf(k::installationUuid, r.base.installationUuid);
    setIf(k::userCountryCode, r.base.userCountryCode);
    setIf(k::serverCountryCode, r.base.serverCountryCode);
    setIf(k::serviceType, r.base.serviceType);
    setIf(k::serviceProtocol, r.base.serviceProtocol);
    if (r.base.authData.is_object() && !r.base.authData.empty()) {
        p[k::authData] = r.base.authData;
    }
    // cli_version и subscription_status кладутся всегда (как в оригинале).
    p[k::cliVersion] = r.cliVersion;
    p[k::subscriptionStatus] = r.subscriptionStatus;
    return p;
}

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

} // namespace agw::services
