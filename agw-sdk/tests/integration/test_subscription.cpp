#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "agw/config.h"
#include "agw/gateway_controller.h"
#include "mock_gateway/mock_gateway.h"
#include "services/api_methods.h"
#include "services/server_config.h"
#include "util/base64.h"
#include "util/checksum.h"
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
        services::GatewayRequest r;
        r.osVersion = "macos";
        r.appVersion = "4.9.0";
        r.appLanguage = "en";
        r.installationUuid = "uuid-1";
        r.userCountryCode = "US";
        r.serviceType = "amnezia-premium";
        r.serviceProtocol = "awg";
        return r;
    };

    // --- importService: успех, распаковка config, плейсхолдер цел ---------
    {
        const std::string cfg = R"({"config_version":2,"awg":{"client_priv_key":"$WIREGUARD_CLIENT_PRIVATE_KEY"}})";
        const std::string configField = "vpn://" + util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));

        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"config", configField}}.dump();
        GatewayController gw = makeClient(mock);

        auto res = services::importService(gw, baseReq(), "WG_PUB_KEY");
        CHECK(res.error == ErrorCode::NoError);
        CHECK(!res.captchaRequired);
        CHECK_EQ(res.serverConfigJson, cfg);  // распаковано, плейсхолдер $WIREGUARD_CLIENT_PRIVATE_KEY цел

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("public_key", std::string()), std::string("WG_PUB_KEY"));
        CHECK_EQ(sent.value("service_protocol", std::string()), std::string("awg"));
        CHECK_EQ(sent.value("installation_uuid", std::string()), std::string("uuid-1"));
    }

    // --- importService: требуется капча -----------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":402,"captcha_id":"cid","captcha_image":"IMG","hint":"digits"})";
        GatewayController gw = makeClient(mock);

        auto res = services::importService(gw, baseReq(), "WG_PUB_KEY");
        CHECK(res.error == ErrorCode::ApiCaptchaRequiredError);
        CHECK(res.captchaRequired);
        CHECK_EQ(res.captcha.captchaId, std::string("cid"));
        CHECK_EQ(res.captcha.captchaImageBase64, std::string("IMG"));
        CHECK_EQ(res.captcha.hint, std::string("digits"));
    }

    // --- resolveImportCaptcha: нормализация решения + payload --------------
    {
        const std::string cfg = R"({"config_version":2})";
        const std::string configField = "vpn://" + util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"config", configField}}.dump();
        GatewayController gw = makeClient(mock);

        // решение с мусором и полноширинными цифрами «１２３» (U+FF11..U+FF13) → "123"
        auto res = services::resolveImportCaptcha(gw, baseReq(), "WG_PUB_KEY", "cid", "a1b2c\xEF\xBC\x93");
        CHECK(res.error == ErrorCode::NoError);
        CHECK_EQ(res.serverConfigJson, cfg);

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("captcha_id", std::string()), std::string("cid"));
        CHECK_EQ(sent.value("captcha_solution", std::string()), std::string("123"));  // 1,2 ASCII + ３→3
    }

    // --- importTrial: email в payload -------------------------------------
    {
        const std::string cfg = R"({"config_version":2})";
        const std::string configField = "vpn://" + util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"config", configField}}.dump();
        GatewayController gw = makeClient(mock);

        auto res = services::importTrial(gw, baseReq(), "WG_PUB_KEY", "user@example.com");
        CHECK(res.error == ErrorCode::NoError);
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("email", std::string()), std::string("user@example.com"));
    }

    // --- deactivateDevice: NotFound трактуется приложением ----------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":404})";
        GatewayController gw = makeClient(mock);
        ErrorCode ec = services::deactivateDevice(gw, baseReq());
        CHECK(ec == ErrorCode::ApiNotFoundError);  // SDK отдаёт как есть; app трактует как успех
    }

    // --- configTypeFromJson: gateway-подмножество -------------------------
    {
        using services::configTypeFromJson;
        using services::GatewayConfigType;
        const std::string freeV2 = "free.v2.ep";
        const std::string premV1 = "prem.v1.ep";

        auto cv2 = [](const char *st) {
            return util::Json{{"config_version", 2}, {"api_config", {{"service_type", st}}}};
        };
        CHECK(configTypeFromJson(cv2("amnezia-premium"), freeV2, premV1) == GatewayConfigType::AmneziaPremiumV2);
        CHECK(configTypeFromJson(cv2("amnezia-free"), freeV2, premV1) == GatewayConfigType::AmneziaFreeV3);
        CHECK(configTypeFromJson(cv2("external-premium"), freeV2, premV1) == GatewayConfigType::ExternalPremium);

        CHECK(configTypeFromJson(util::Json{{"config_version", 1}, {"api_endpoint", "https://prem.v1.ep/x"}}, freeV2, premV1)
              == GatewayConfigType::AmneziaPremiumV1);
        CHECK(configTypeFromJson(util::Json{{"config_version", 1}, {"api_endpoint", "https://free.v2.ep/x"}}, freeV2, premV1)
              == GatewayConfigType::AmneziaFreeV2);
        // telegram (v1) без совпадения эндпоинта, но с gateway service_type → fallthrough
        CHECK(configTypeFromJson(util::Json{{"config_version", 1}, {"api_config", {{"service_type", "amnezia-premium"}}}}, freeV2, premV1)
              == GatewayConfigType::AmneziaPremiumV2);
        // self-hosted/прочее → Unknown
        CHECK(configTypeFromJson(util::Json{{"config_version", 0}}, freeV2, premV1) == GatewayConfigType::Unknown);
    }

    // --- qtChecksum16: эталон CRC-16/X-25 (== Qt qChecksum ISO3309) -------
    {
        // Каноничный check-вектор каталога CRC: "123456789" → 0x906E. Независимая сверка с Qt.
        CHECK(util::qtChecksum16(std::string("123456789")) == 0x906E);
        CHECK(util::qtChecksum16(std::string("")) == 0x0000);
    }

    // --- importServiceFromAppStore: успех, key→config, crc, vpnKey --------
    {
        const std::string cfg = R"({"config_version":2,"awg":{"client_priv_key":"$WIREGUARD_CLIENT_PRIVATE_KEY"}})";
        const std::string normalizedKey = util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));

        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"key", "vpn://" + normalizedKey}}.dump();
        GatewayController gw = makeClient(mock);

        auto res = services::importServiceFromAppStore(gw, baseReq(), "WG_PUB_KEY", "txn-123");
        CHECK(res.error == ErrorCode::NoError);
        CHECK_EQ(res.serverConfigJson, cfg);
        CHECK_EQ(res.vpnKey, normalizedKey);  // без "vpn://"
        // crc считается над Qt-indented JSON распарсенного конфига
        const std::uint16_t expectCrc = util::qtChecksum16(util::qtIndentedDump(util::Json::parse(cfg)));
        CHECK(res.crc == expectCrc);
        CHECK(res.crc != 0);

        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("transaction_id", std::string()), std::string("txn-123"));
        CHECK_EQ(sent.value("public_key", std::string()), std::string("WG_PUB_KEY"));
    }

    // --- importServiceFromAppStore: пустой key → ApiPurchaseError ----------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"key":""})";
        GatewayController gw = makeClient(mock);
        auto res = services::importServiceFromAppStore(gw, baseReq(), "WG_PUB_KEY", "txn-1");
        CHECK(res.error == ErrorCode::ApiPurchaseError);
    }

    // --- importServiceFromAppStore: чужой config_version → ApiPurchaseError -
    {
        const std::string cfg = R"({"config_version":1})";  // не AmneziaGateway(2)
        const std::string normalizedKey = util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = util::Json{{"key", "vpn://" + normalizedKey}}.dump();
        GatewayController gw = makeClient(mock);
        auto res = services::importServiceFromAppStore(gw, baseReq(), "WG_PUB_KEY", "txn-1");
        CHECK(res.error == ErrorCode::ApiPurchaseError);
    }

    return AGW_TEST_MAIN_RETURN();
}
