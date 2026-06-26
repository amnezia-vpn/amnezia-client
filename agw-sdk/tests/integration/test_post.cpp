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
}

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
    const std::string endpoint = "https://%1/api/v1/test";
    const FailoverContext ctx{"premium", "US"};
    const std::string payload = R"({"hello":"world","n":42})";

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

        CHECK_EQ(mock->lastDecryptedPayload, payload);

        CHECK_EQ(mock->lastUrl, std::string("https://gw.example.test/api/v1/test"));
        CHECK_EQ(seenHost, std::string("gw.example.test"));
        CHECK(mock->requestCount == 1);

        CHECK(mock->lastRequestId.size() == 36);
        CHECK(mock->lastRequestId[14] == '4');
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"http_status":409,"message":"limit"})";
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigLimitError);

        CHECK_EQ(r.body, std::string(R"({"http_status":409,"message":"limit"})"));
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->simulateSsl = true;
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigSslError);
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->simulateTransport = TransportError::Timeout;
        GatewayController client(baseConfig(mock, pub));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiConfigTimeoutError);
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        Config cfg = baseConfig(mock, "not a pem key");
        GatewayController client(std::move(cfg));
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::ApiMissingAgwPublicKey);

        CHECK(mock->requestCount == 0);
    }

    return AGW_TEST_MAIN_RETURN();
}
