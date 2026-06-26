#include "agw_test.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "agw/cancellation.h"
#include "agw/gateway_controller.h"
#include "agw/config.h"
#include "crypto/aes.h"
#include "crypto/rsa.h"
#include "mock_gateway/mock_gateway.h"
#include "protocol/keys.h"
#include "util/base64.h"
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

Config baseConfig(std::shared_ptr<IHttpClient> http, const std::string &pub)
{
    Config c;
    c.gatewayEndpoint = "gw.example.test";
    c.agwPublicKeyPem = pub;
    c.requestTimeoutMsecs = 5000;
    c.httpClient = std::move(http);
    return c;
}

class BlockingUntilCancelMock : public IHttpClient {
public:
    std::atomic<int> entered{0};
    HttpResponse send(const HttpRequest &req) override
    {
        entered.fetch_add(1);
        while (!(req.cancelCheck && req.cancelCheck())) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        HttpResponse r;
        r.error = TransportError::Canceled;
        return r;
    }
};

class StatelessMock : public IHttpClient {
public:
    explicit StatelessMock(std::string priv) : m_priv(std::move(priv)) {}
    std::atomic<int> count{0};
    HttpResponse send(const HttpRequest &req) override
    {
        count.fetch_add(1);
        namespace k = protocol::keys;
        util::Json body = util::Json::parse(req.body);
        const auto keyCipher = util::base64Decode(body[k::keyPayload].get<std::string>());
        const auto keysBytes = crypto::rsaDecryptPrivatePkcs1(keyCipher, m_priv);
        util::Json keysJson = util::Json::parse(std::string(keysBytes.begin(), keysBytes.end()));
        const auto aesKey = util::base64Decode(keysJson[k::aesKey].get<std::string>());
        const auto aesIv = util::base64Decode(keysJson[k::aesIv].get<std::string>());

        const std::string plain = R"({"ok":true})";
        const std::vector<std::uint8_t> pv(plain.begin(), plain.end());
        const auto cipher = crypto::aesEncryptCbc(pv, aesKey, aesIv);
        HttpResponse r;
        r.httpStatusCode = 200;
        r.body.assign(cipher.begin(), cipher.end());
        return r;
    }

private:
    std::string m_priv;
};
}

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
    const std::string endpoint = "https://%1/api/v1/test";
    const FailoverContext ctx{"prem", "US"};
    const std::string payload = R"({"hello":"world"})";

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"ok":true,"v":1})";
        GatewayController client(baseConfig(mock, pub));
        std::future<Response> f = client.postFuture(endpoint, payload, ctx);
        Response r = f.get();
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.body, std::string(R"({"ok":true,"v":1})"));
        CHECK_EQ(mock->lastDecryptedPayload, payload);
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        mock->responsePlain = R"({"async":true})";
        GatewayController client(baseConfig(mock, pub));

        std::promise<Response> p;
        std::future<Response> f = p.get_future();
        client.postAsync(
            endpoint, payload, [&p](Response r) { p.set_value(std::move(r)); }, ctx);
        Response r = f.get();
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.body, std::string(R"({"async":true})"));
    }

    {
        auto mock = std::make_shared<agw_test::MockGateway>(priv);
        GatewayController client(baseConfig(mock, pub));
        CancellationToken token;
        token.cancel();
        std::future<Response> f = client.postFuture(endpoint, payload, ctx, &token);
        Response r = f.get();
        CHECK(r.error == ErrorCode::Cancelled);
        CHECK(mock->requestCount == 0);
    }

    {
        auto mock = std::make_shared<BlockingUntilCancelMock>();
        GatewayController client(baseConfig(mock, pub));
        CancellationToken token;

        std::promise<Response> p;
        std::future<Response> f = p.get_future();
        client.postAsync(
            endpoint, payload, [&p](Response r) { p.set_value(std::move(r)); }, ctx, &token);

        while (mock->entered.load() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        token.cancel();
        Response r = f.get();
        CHECK(r.error == ErrorCode::Cancelled);
    }

    {
        auto mock = std::make_shared<StatelessMock>(priv);
        Config cfg = baseConfig(mock, pub);
        cfg.threadPoolSize = 8;
        GatewayController client(std::move(cfg));

        constexpr int N = 64;
        std::vector<std::future<Response>> futs;
        futs.reserve(N);
        for (int i = 0; i < N; ++i) {
            futs.push_back(client.postFuture(endpoint, payload, ctx));
        }
        int ok = 0;
        for (auto &fut : futs) {
            Response r = fut.get();
            if (r.error == ErrorCode::NoError && r.body == R"({"ok":true})") {
                ++ok;
            }
        }
        CHECK(ok == N);
        CHECK(mock->count.load() == N);
    }

    return AGW_TEST_MAIN_RETURN();
}
