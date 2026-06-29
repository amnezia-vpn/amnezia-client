#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "agw/c_abi.h"
#include "agw/types.h"
#include "detail/test_hooks.h"
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

std::string g_pub, g_priv;

// Создаёт клиент с mock-шлюзом, который расшифрует запрос и отдаст responsePlain (зашифровав обратно).
agw_client *makeClient(const std::string &responsePlain, std::shared_ptr<agw_test::MockGateway> &mockOut)
{
    auto mock = std::make_shared<agw_test::MockGateway>(g_priv);
    mock->responsePlain = responsePlain;
    mockOut = mock;
    agw::detail::setNextTestHttpClient(mock);

    agw_config cfg{};
    cfg.gateway_endpoint = "gw.example.test";
    cfg.agw_public_key_pem = g_pub.c_str();
    cfg.request_timeout_msecs = 5000;
    return agw_client_create(&cfg);
}

agw_gateway_request baseReq()
{
    agw_gateway_request r{};
    r.os_version = "macos";
    r.app_version = "4.9.0";
    r.app_language = "en";
    r.installation_uuid = "uuid-1";
    r.user_country_code = "US";
    r.service_type = "amnezia-premium";
    r.service_protocol = "awg";
    return r;
}

} // namespace

int main()
{
    g_pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    g_priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");

    // --- agw_get_services -------------------------------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(R"({"services":[{"id":1}]})", mock);
        CHECK(c != nullptr);
        agw_json_result r = agw_get_services(c, "macos", "4.9.0", "amnezia", "en");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK(r.json != nullptr);
        util::Json doc = util::Json::parse(std::string(r.json, r.json_len));
        CHECK(doc.contains("services"));
        // payload корректно собран (mock расшифровал)
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("cli_name", std::string()), std::string("amnezia"));
        agw_json_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_get_news (страны/типы как C-массивы) -------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(R"([{"n":1}])", mock);
        const char *countries[] = { "US", "DE" };
        const char *types[] = { "amnezia-premium" };
        agw_json_result r = agw_get_news(c, "en", countries, 2, types, 1);
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        util::Json doc = util::Json::parse(std::string(r.json, r.json_len));
        CHECK(doc.is_array());
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK(sent["user_country_code"].is_array());
        CHECK_EQ(sent["user_country_code"][1].get<std::string>(), std::string("DE"));
        agw_json_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_get_updater_endpoint (url + raw) -----------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(R"({"url":"https://upd.example/"})", mock);
        agw_url_result r = agw_get_updater_endpoint(c, "4.9.0", "macos", "uuid-1");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK_EQ(std::string(r.url ? r.url : ""), std::string("https://upd.example"));  // хвостовой '/' срезан
        CHECK(r.raw_json != nullptr);
        agw_url_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_get_account_info (плоский agw_api_config) --------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(
                R"({"service_type":"amnezia-premium","active_device_count":2,"max_device_count":5,
                    "subscription":{"end_date":"2030-01-01T00:00:00Z"},
                    "service_info":{"is_ad_visible":true,"ad_header":"H"}})",
                mock);
        agw_gateway_request req = baseReq();
        agw_account_info_result r = agw_get_account_info(c, &req, "4.9.0", "active");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK_EQ(std::string(r.account.service_type ? r.account.service_type : ""), std::string("amnezia-premium"));
        CHECK(r.account.active_device_count == 2);
        CHECK(r.account.max_device_count == 5);
        CHECK_EQ(std::string(r.account.subscription_end_date ? r.account.subscription_end_date : ""),
                 std::string("2030-01-01T00:00:00Z"));
        CHECK(r.account.is_ad_visible == 1);
        CHECK_EQ(std::string(r.account.ad_header ? r.account.ad_header : ""), std::string("H"));
        // payload содержит cli_version + subscription_status
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("cli_version", std::string()), std::string("4.9.0"));
        CHECK_EQ(sent.value("subscription_status", std::string()), std::string("active"));
        agw_account_info_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_get_renewal_link --------------------------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(R"({"renewal_url":"https://pay.example/x"})", mock);
        agw_gateway_request req = baseReq();
        agw_url_result r = agw_get_renewal_link(c, &req, "4.9.0", "active");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK_EQ(std::string(r.url ? r.url : ""), std::string("https://pay.example/x"));
        agw_url_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_import_service (успех → server_config_json) ------------------
    {
        const std::string cfg = R"({"config_version":2,"awg":{"k":"$WIREGUARD_CLIENT_PRIVATE_KEY"}})";
        const std::string configField = "vpn://" + util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(util::Json{{"config", configField}}.dump(), mock);
        agw_gateway_request req = baseReq();
        agw_import_result r = agw_import_service(c, &req, "WG_PUB");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK(r.captcha_required == 0);
        CHECK_EQ(std::string(r.server_config_json ? r.server_config_json : ""), cfg);
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("public_key", std::string()), std::string("WG_PUB"));
        agw_import_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_import_service (капча) ---------------------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(
                R"({"http_status":402,"captcha_id":"cid","captcha_image":"IMG","hint":"h"})", mock);
        agw_gateway_request req = baseReq();
        agw_import_result r = agw_import_service(c, &req, "WG_PUB");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::ApiCaptchaRequiredError));
        CHECK(r.captcha_required == 1);
        CHECK_EQ(std::string(r.captcha_id ? r.captcha_id : ""), std::string("cid"));
        CHECK_EQ(std::string(r.captcha_image ? r.captcha_image : ""), std::string("IMG"));
        agw_import_result_free(&r);
        agw_client_destroy(c);
    }

    // --- agw_deactivate_device -------------------------------------------
    {
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(R"({"http_status":404})", mock);
        agw_gateway_request req = baseReq();
        int ec = agw_deactivate_device(c, &req);
        CHECK(ec == static_cast<int>(agw::ErrorCode::ApiNotFoundError));
        agw_client_destroy(c);
    }

    // --- agw_import_from_app_store (key→config, vpnKey, crc) --------------
    {
        const std::string cfg = R"({"config_version":2})";
        const std::string normalizedKey = util::base64UrlEncodeNoPad(util::qtCompress(bytesOf(cfg), 6));
        std::shared_ptr<agw_test::MockGateway> mock;
        agw_client *c = makeClient(util::Json{{"key", "vpn://" + normalizedKey}}.dump(), mock);
        agw_gateway_request req = baseReq();
        agw_app_store_result r = agw_import_from_app_store(c, &req, "WG_PUB", "txn-9");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::NoError));
        CHECK_EQ(std::string(r.server_config_json ? r.server_config_json : ""), cfg);
        CHECK_EQ(std::string(r.vpn_key ? r.vpn_key : ""), normalizedKey);
        CHECK(r.crc != 0);
        util::Json sent = util::Json::parse(mock->lastDecryptedPayload);
        CHECK_EQ(sent.value("transaction_id", std::string()), std::string("txn-9"));
        agw_app_store_result_free(&r);
        agw_client_destroy(c);
    }

    // --- null-guard -------------------------------------------------------
    {
        agw_import_result r = agw_import_service(nullptr, nullptr, "x");
        CHECK(r.error == static_cast<int>(agw::ErrorCode::ApiConfigDownloadError));
        agw_import_result_free(&r);
        CHECK(agw_deactivate_device(nullptr, nullptr) == static_cast<int>(agw::ErrorCode::ApiConfigDownloadError));
    }

    return AGW_TEST_MAIN_RETURN();
}
