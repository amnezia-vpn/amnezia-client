#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "agw/gateway_controller.h"
#include "agw/config.h"
#include "mock_gateway/mock_gateway.h"

using namespace agw;

namespace {

std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

Config baseConfig(std::shared_ptr<IHttpClient> http, const std::string &pubPem)
{
    Config c;
    c.gatewayEndpoint = "gw.example.test";
    c.agwPublicKeyPem = pubPem;
    c.requestTimeoutMsecs = 5000;
    c.httpClient = std::move(http);
    return c;
}

} // namespace

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
    const std::string endpoint = "https://%1/api/v1/test";
    const FailoverContext ctx{"premium", "US"};
    const std::string payload = R"({"hello":"world","n":42})";

    // --- happy path: round-trip через мок-шлюз -----------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"ok":true,"data":"hi"})";

        std::string seenHost;
        Config cfg = baseConfig(mock, pub);
        cfg.onBeforeRequest = [&](const std::string &h) { seenHost = h; };

        GatewayController client(std::move(cfg));
        Response r = client.post(endpoint, payload, ctx);

        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.body, std::string(R"({"ok":true,"data":"hi"})"));
        // сервер увидел ровно наш payload (api_payload расшифровался)
        CHECK_EQ(mock->lastDecryptedPayload, payload);
        // url собран из endpoint + host; хук получил хост один раз
        CHECK_EQ(mock->lastUrl, std::string("https://gw.example.test/api/v1/test"));
        CHECK_EQ(seenHost, std::string("gw.example.test"));
        CHECK(mock->requestCount == 1);
        // request-id выставлен (UUID v4 формы)
        CHECK(mock->lastRequestId.size() == 36);
        CHECK(mock->lastRequestId[14] == '4');
    }

    // --- ошибка из тела (http_status в зашифрованном ответе) ---------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":409,"message":"limit"})";
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigLimitError);
        // тело несётся и при ошибке
        CHECK_EQ(r.body, std::string(R"({"http_status":409,"message":"limit"})"));
    }

    // --- SSL-ошибка транспорта --------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->simulateSsl = true;
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigSslError);
    }

    // --- таймаут транспорта -----------------------------------------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->simulateTransport = TransportError::Timeout;
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigTimeoutError);
    }

    // --- невалидный публичный ключ → ApiMissingAgwPublicKey ----------------
    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        Config cfg = baseConfig(mock, "not a pem key");
        GatewayController client(std::move(cfg));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiMissingAgwPublicKey);
        // до сети дело не дошло
        CHECK(mock->requestCount == 0);
    }

    return AGW_TEST_MAIN_RETURN();
}
