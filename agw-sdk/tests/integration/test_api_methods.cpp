#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "agw/config.h"
#include "agw/gateway_controller.h"
#include "mock_gateway/mock_gateway.h"
#include "services/api_methods.h"
#include "util/json.h"

using namespace agw;

namespace {

std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

} // namespace

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");

    auto makeClient = [&](std::shared_ptr<agw_test::MockGateway> mock) {
        Config c;
        c.gatewayEndpoint = "http://gw.example.test/";
        c.agwPublicKeyPem = pub;
        c.isDevEnvironment = true;
        c.requestTimeoutMsecs = 5000;
        c.httpClient = mock;
        return GatewayController(std::move(c));
    };

    // --- getServices: payload + разбор ------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"services":[{"service_type":"amnezia-premium"}]})";
        GatewayController gw = makeClient(mock);

        services::ServicesRequest req{"macos", "4.9.0", "amnezia", "en"};
        auto res = services::getServices(gw, req);

        CHECK(res.error == ErrorCode::NoError);
        CHECK(res.value.contains("services"));
        CHECK(res.value["services"].is_array());
        CHECK(res.value["services"].size() == 1);

        // сервер увидел корректный payload
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("os_version", std::string()), std::string("macos"));
        CHECK_EQ(sent.value("app_version", std::string()), std::string("4.9.0"));
        CHECK_EQ(sent.value("cli_name", std::string()), std::string("amnezia"));
        CHECK_EQ(sent.value("app_language", std::string()), std::string("en"));
    }

    // --- getServices: нет "services" → ApiServicesMissingError ------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"foo":1})";
        GatewayController gw = makeClient(mock);
        auto res = services::getServices(gw, {"macos", "4.9.0", "amnezia", "en"});
        CHECK(res.error == ErrorCode::ApiServicesMissingError);
    }

    // --- getNews: {news:[...]} и payload с массивами ----------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"news":[{"id":1},{"id":2}]})";
        GatewayController gw = makeClient(mock);

        services::NewsRequest req;
        req.locale = "en";
        req.userCountryCodes = {"US", "DE"};
        req.serviceTypes = {"amnezia-premium"};
        auto res = services::getNews(gw, req);

        CHECK(res.error == ErrorCode::NoError);
        CHECK(res.value.is_array());
        CHECK(res.value.size() == 2);

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("locale", std::string()), std::string("en"));
        CHECK(sent["user_country_code"].is_array() && sent["user_country_code"].size() == 2);
        CHECK(sent["service_type"].is_array() && sent["service_type"].size() == 1);
    }

    // --- getNews: голый массив в ответе -----------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"([{"id":9}])";
        GatewayController gw = makeClient(mock);
        auto res = services::getNews(gw, {"en", {}, {}});
        CHECK(res.error == ErrorCode::NoError);
        CHECK(res.value.is_array() && res.value.size() == 1);
    }

    // --- getUpdaterEndpoint: url с обрезкой '/' ---------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"url":"https://updates.example/"})";
        GatewayController gw = makeClient(mock);

        services::UpdaterRequest req{"4.9.0", "macos", "uuid-123"};
        auto res = services::getUpdaterEndpoint(gw, req);

        CHECK(res.error == ErrorCode::NoError);
        CHECK_EQ(res.url, std::string("https://updates.example"));  // '/' обрезан

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("cli_version", std::string()), std::string("4.9.0"));
        CHECK_EQ(sent.value("installation_uuid", std::string()), std::string("uuid-123"));
    }

    // --- getAccountInfo: парсинг в ApiConfig + payload --------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({
            "service_type":"amnezia-premium",
            "subscription":{"end_date":"2030-01-01T00:00:00Z"},
            "active_device_count":2,"max_device_count":5,"issued_configs":3,
            "service_info":{"is_ad_visible":false,"is_renewal_available":true,"ad_header":"H"},
            "public_key":{"expires_at":"2031-01-01T00:00:00Z"},
            "available_countries":[{"code":"US"}],
            "is_in_app_purchase":true
        })";
        GatewayController gw = makeClient(mock);

        services::AccountRequest req;
        req.base.osVersion = "macos";
        req.base.appVersion = "4.9.0";
        req.base.appLanguage = "en";
        req.base.installationUuid = "uuid-1";
        req.base.userCountryCode = "US";
        req.base.serviceType = "amnezia-premium";
        req.base.authData = util::Json{{"api_key", "AK"}};
        req.cliVersion = "4.9.0";
        req.subscriptionStatus = "active";

        auto res = services::getAccountInfo(gw, req);
        CHECK(res.error == ErrorCode::NoError);
        CHECK_EQ(res.account.serviceType, std::string("amnezia-premium"));
        CHECK_EQ(res.account.subscriptionEndDate, std::string("2030-01-01T00:00:00Z"));
        CHECK(res.account.activeDeviceCount == 2);
        CHECK(res.account.maxDeviceCount == 5);
        CHECK(res.account.issuedConfigs == 3);
        CHECK(res.account.serviceInfo.isRenewalAvailable == true);
        CHECK_EQ(res.account.serviceInfo.adHeader, std::string("H"));
        CHECK_EQ(res.account.publicKeyExpiresAt, std::string("2031-01-01T00:00:00Z"));
        CHECK(res.account.isInAppPurchase == true);
        CHECK(!res.account.availableCountriesJson.empty());

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("installation_uuid", std::string()), std::string("uuid-1"));
        CHECK_EQ(sent.value("cli_version", std::string()), std::string("4.9.0"));
        CHECK_EQ(sent.value("subscription_status", std::string()), std::string("active"));
        CHECK(sent.contains("auth_data") && sent["auth_data"].value("api_key", std::string()) == "AK");
        CHECK_EQ(sent.value("service_type", std::string()), std::string("amnezia-premium"));
    }

    // --- getRenewalLink ----------------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"renewal_url":"https://renew.example/x"})";
        GatewayController gw = makeClient(mock);

        services::AccountRequest req;
        req.base.osVersion = "macos";
        req.base.serviceType = "amnezia-premium";
        req.cliVersion = "4.9.0";
        req.subscriptionStatus = "expired";

        auto res = services::getRenewalLink(gw, req);
        CHECK(res.error == ErrorCode::NoError);
        CHECK_EQ(res.renewalUrl, std::string("https://renew.example/x"));
    }

    return AGW_TEST_MAIN_RETURN();
}
