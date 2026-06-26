#include "services/models.h"

namespace agw::services {

namespace {

std::string str(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

int integer(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_number_integer()) ? it->get<int>() : 0;
}

bool boolean(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_boolean()) ? it->get<bool>() : false;
}

util::Json objectAt(const util::Json &j, const char *key)
{
    auto it = j.find(key);
    return (it != j.end() && it->is_object()) ? *it : util::Json::object();
}

} // namespace

ApiConfig parseApiConfig(const util::Json &json)
{
    ApiConfig c;
    if (!json.is_object()) {
        return c;
    }

    c.serviceType = str(json, "service_type");
    c.serviceProtocol = str(json, "service_protocol");
    c.userCountryCode = str(json, "user_country_code");
    c.serverCountryCode = str(json, "server_country_code");
    c.serverCountryName = str(json, "server_country_name");
    c.vpnKey = str(json, "vpn_key");

    const util::Json sub = objectAt(json, "subscription");
    c.subscriptionEndDate = str(sub, "end_date");

    c.activeDeviceCount = integer(json, "active_device_count");
    c.maxDeviceCount = integer(json, "max_device_count");
    c.issuedConfigs = integer(json, "issued_configs");

    if (auto it = json.find("available_countries"); it != json.end() && it->is_array()) {
        c.availableCountriesJson = it->dump();
    }
    if (auto it = json.find("supported_protocols"); it != json.end() && it->is_array()) {
        c.supportedProtocolsJson = it->dump();
    }

    const util::Json si = objectAt(json, "service_info");
    c.serviceInfo.isAdVisible = boolean(si, "is_ad_visible");
    c.serviceInfo.isRenewalAvailable = boolean(si, "is_renewal_available");
    c.serviceInfo.adHeader = str(si, "ad_header");
    c.serviceInfo.adDescription = str(si, "ad_description");
    c.serviceInfo.adEndpoint = str(si, "ad_endpoint");

    const util::Json pk = objectAt(json, "public_key");
    c.publicKeyExpiresAt = str(pk, "expires_at");

    c.stackType = str(json, "stack_type");
    c.cliVersion = str(json, "cli_version");
    c.isTestPurchase = boolean(json, "is_test_purchase");
    c.isInAppPurchase = boolean(json, "is_in_app_purchase");
    c.subscriptionExpiredByServer = boolean(json, "subscription_expired_by_server");

    return c;
}

} // namespace agw::services
