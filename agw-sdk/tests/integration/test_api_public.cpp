#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "agw/api.h"
#include "agw/config.h"
#include "agw/gateway_controller.h"
#include "mock_gateway/mock_gateway.h"
#include "util/base64.h"
#include "util/json.h"
#include "util/zlib.h"

using namespace agw;

namespace {

std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::vector<std::uint8_t> bytesOf(const std::string &s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
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

    auto baseReq = [] {
        api::GatewayRequest r;
        r.osVersion = "macos";
        r.appVersion = "4.9.0";
        r.appLanguage = "en";
        r.installationUuid = "uuid-1";
        r.userCountryCode = "US";
        r.serviceType = "amnezia-premium";
        r.serviceProtocol = "awg";
        return r;
    };

    // --- getServices: json текстом ----------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"services":[{"id":1}]})";
        GatewayController gw = makeClient(mock);
        api::JsonResult r = api::getServices(gw, "macos", "4.9.0", "amnezia", "en");
        CHECK(r.error == ErrorCode::NoError);
        util::Json doc = util::Json::parse(r.json);
        CHECK(doc.contains("services"));
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("cli_name", std::string()), std::string("amnezia"));
    }

    // --- getNews: массив + страны/типы параметрами ------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"([{"n":1}])";
        GatewayController gw = makeClient(mock);
        api::JsonResult r = api::getNews(gw, "en", { "US", "DE" }, { "amnezia-premium" });
        CHECK(r.error == ErrorCode::NoError);
        CHECK(util::Json::parse(r.json).is_array());
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK(sent["user_country_code"].is_array());
    }

    // --- getUpdaterEndpoint: url + rawJson --------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"url":"https://upd.example/"})";
        GatewayController gw = makeClient(mock);
        api::UrlResult r = api::getUpdaterEndpoint(gw, "4.9.0", "macos", "uuid-1");
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.url, std::string("https://upd.example"));
        CHECK(!r.rawJson.empty());
    }

    // --- getAccountInfo: ApiConfig (std) ----------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"service_type":"amnezia-premium","active_device_count":2,
                                   "subscription":{"end_date":"2030-01-01T00:00:00Z"}})";
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        api::AccountInfoResult r = api::getAccountInfo(gw, req, "4.9.0", "active");
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.account.serviceType, std::string("amnezia-premium"));
        CHECK(r.account.activeDeviceCount == 2);
        CHECK_EQ(r.account.subscriptionEndDate, std::string("2030-01-01T00:00:00Z"));
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("subscription_status", std::string()), std::string("active"));
    }

    // --- getRenewalLink ---------------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"renewal_url":"https://pay.example/x"})";
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        api::RenewalResult r = api::getRenewalLink(gw, req, "4.9.0", "active");
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.renewalUrl, std::string("https://pay.example/x"));
    }

    // --- importService: успех + authDataJson прокинут --------------------
    {
        const std::string cfg = R"({"config_version":2,"awg":{"k":"$WIREGUARD_CLIENT_PRIVATE_KEY"}})";
        const std::string configField = "vpn://" + util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"config", configField}}.dump();
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        req.authDataJson = R"({"api_key":"AK"})";
        api::ImportResult r = api::importService(gw, req, "WG_PUB");
        CHECK(r.error == ErrorCode::NoError);
        CHECK(!r.captchaRequired);
        CHECK_EQ(r.serverConfigJson, cfg);
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("public_key", std::string()), std::string("WG_PUB"));
        CHECK(sent["auth_data"].is_object());  // authDataJson распарсен в объект
        CHECK_EQ(sent["auth_data"].value("api_key", std::string()), std::string("AK"));
    }

    // --- importService: капча ---------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":402,"captcha_id":"cid","captcha_image":"IMG","hint":"h"})";
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        api::ImportResult r = api::importService(gw, req, "WG_PUB");
        CHECK(r.error == ErrorCode::ApiCaptchaRequiredError);
        CHECK(r.captchaRequired);
        CHECK_EQ(r.captcha.captchaId, std::string("cid"));
    }

    // --- importServiceFromAppStore: config+vpnKey+crc --------------------
    {
        const std::string cfg = R"({"config_version":2})";
        const std::string normalizedKey = util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"key", "vpn://" + normalizedKey}}.dump();
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        api::AppStoreImportResult r = api::importServiceFromAppStore(gw, req, "WG_PUB", "txn-9");
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.serverConfigJson, cfg);
        CHECK_EQ(r.vpnKey, normalizedKey);
        CHECK(r.crc != 0);
    }

    // --- deactivateDevice -------------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":404})";
        GatewayController gw = makeClient(mock);
        api::GatewayRequest req = baseReq();
        CHECK(api::deactivateDevice(gw, req) == ErrorCode::ApiNotFoundError);
    }

    return AGW_TEST_MAIN_RETURN();
}
