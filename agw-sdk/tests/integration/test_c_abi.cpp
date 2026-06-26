#include "agw_test.h"

#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

#include "agw/c_abi.h"
#include "agw/types.h"
#include "detail/test_hooks.h"
#include "mock_gateway/mock_gateway.h"

namespace {
std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

agw_config makeConfig(const char *gateway, const char *pem)
{
    agw_config c{};
    c.gateway_endpoint = gateway;
    c.agw_public_key_pem = pem;
    c.request_timeout_msecs = 5000;
    return c;
}

struct AsyncSink {
    std::promise<std::pair<int, std::string>> promise;
};

void asyncCallback(agw_response r, void *ud)
{
    auto *sink = static_cast<AsyncSink *>(ud);
    sink->promise.set_value({r.error, r.body ? std::string(r.body, r.body_len) : std::string()});
    agw_response_free(&r);
}
}

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
    const std::string payload = R"({"hello":"world"})";

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"ok":true,"c":1})";
        agw::detail::setNextTestHttpClient(mock);

        agw_config cfg = makeConfig("gw.example.test", pub.c_str());
        agw_client *client = agw_client_create(&cfg);
        CHECK(client != nullptr);

        agw_response r = agw_client_post(client, "https://%1/api/v1/test", payload.c_str(), "prem", "US", nullptr);
        CHECK(r.error == 0);
        CHECK(r.body != nullptr);
        CHECK_EQ(std::string(r.body, r.body_len), std::string(R"({"ok":true,"c":1})"));
        CHECK_EQ(mock->lastDecryptedPayload, payload);
        agw_response_free(&r);
        CHECK(r.body == nullptr);

        mock->responsePlain = R"({"async":1})";
        AsyncSink sink;
        auto fut = sink.promise.get_future();
        agw_client_post_async(client, "https://%1/api/v1/test", payload.c_str(), "prem", "US",
                              &asyncCallback, &sink, nullptr);
        auto [err, body] = fut.get();
        CHECK(err == 0);
        CHECK_EQ(body, std::string(R"({"async":1})"));

        agw_client_destroy(client);
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        agw::detail::setNextTestHttpClient(mock);
        agw_config cfg = makeConfig("gw.example.test", pub.c_str());
        agw_client *client = agw_client_create(&cfg);

        agw_cancel_token *token = agw_cancel_token_create();
        CHECK(token != nullptr);
        agw_cancel_token_cancel(token);

        agw_response r = agw_client_post(client, "https://%1/api/v1/test", payload.c_str(), "", "", token);
        CHECK(r.error == static_cast<int>(agw::ErrorCode::Cancelled));
        CHECK(mock->requestCount == 0);
        agw_response_free(&r);

        agw_cancel_token_destroy(token);
        agw_client_destroy(client);
    }

    {
        agw_config cfg = makeConfig("gw.example.test", "not a pem");
        agw_client *client = agw_client_create(&cfg);
        CHECK(client != nullptr);
        agw_response r = agw_client_post(client, "https://%1/x", payload.c_str(), "", "", nullptr);
        CHECK(r.error == static_cast<int>(agw::ErrorCode::ApiMissingAgwPublicKey));
        agw_response_free(&r);
        agw_client_destroy(client);
    }

    {
        CHECK(agw_client_create(nullptr) == nullptr);
        agw_response r = agw_client_post(nullptr, "e", "p", "", "", nullptr);
        CHECK(r.error != 0);
        agw_response_free(&r);
        agw_client_destroy(nullptr);
    }

    return AGW_TEST_MAIN_RETURN();
}
