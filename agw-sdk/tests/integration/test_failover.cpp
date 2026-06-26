#include "agw_test.h"

#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "agw/gateway_controller.h"
#include "agw/config.h"
#include "crypto/aes.h"
#include "crypto/rsa.h"
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

bool contains(const std::string &h, const std::string &n) { return h.find(n) != std::string::npos; }

// Маршрутизирующий мок: прямой POST на шлюз «подозрителен», S3 отдаёт список с живым прокси,
// health отвечает 200, POST на прокси — хороший ответ.
class FailoverMock : public IHttpClient {
public:
    explicit FailoverMock(std::string priv) : m_priv(std::move(priv)) {}

    int directPosts = 0, proxyPosts = 0, storageGets = 0, healthGets = 0;

    HttpResponse send(const HttpRequest &req) override
    {
        HttpResponse resp;
        resp.httpStatusCode = 200;

        if (req.method == "GET") {
            if (contains(req.url, "lmbd-health")) {
                ++healthGets;
                if (!contains(req.url, "proxy.good.test")) {
                    resp.error = TransportError::ConnectionError; // нездоровый прокси
                }
                return resp;
            }
            // S3-хранилище: dev-открытый JSON-массив с живым прокси
            ++storageGets;
            resp.body = R"(["https://proxy.good.test/"])";
            return resp;
        }

        // POST: расшифровать ключи и вернуть зашифрованный ответ тем же key/iv
        namespace k = protocol::keys;
        util::Json body = util::Json::parse(req.body);
        const auto keyCipher = util::base64Decode(body[k::keyPayload].get<std::string>());
        const auto keysBytes = crypto::rsaDecryptPrivatePkcs1(keyCipher, m_priv);
        util::Json keysJson = util::Json::parse(std::string(keysBytes.begin(), keysBytes.end()));
        const auto aesKey = util::base64Decode(keysJson[k::aesKey].get<std::string>());
        const auto aesIv = util::base64Decode(keysJson[k::aesIv].get<std::string>());

        std::string plain;
        if (contains(req.url, "proxy.good.test")) {
            ++proxyPosts;
            plain = R"({"ok":true,"via":"proxy"})";
        } else {
            ++directPosts;
            plain = R"({"http_status":404,"message":"blocked"})"; // 404 без паттерна → байпас
        }
        const std::vector<std::uint8_t> pv(plain.begin(), plain.end());
        const auto cipher = crypto::aesEncryptCbc(pv, aesKey, aesIv);
        resp.body.assign(cipher.begin(), cipher.end());
        return resp;
    }

private:
    std::string m_priv;
};

} // namespace

int main()
{
    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");

    auto mock = std::make_shared<FailoverMock>(priv);

    Config cfg;
    cfg.gatewayEndpoint = "https://gw.example.test/";
    cfg.agwPublicKeyPem = pub;
    cfg.isDevEnvironment = true;  // S3-список — открытый
    cfg.s3PrimaryEndpoints = {"https://s3.example.test/"};
    cfg.requestTimeoutMsecs = 5000;
    cfg.httpClient = mock;

    GatewayController client(std::move(cfg));
    const std::string endpoint = "%1api/v1/test";
    const FailoverContext ctx{"prem", "US"};
    const std::string payload = R"({"hello":"world"})";

    // --- post1: прямой подозрителен → failover через прокси ----------------
    {
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.body, std::string(R"({"ok":true,"via":"proxy"})"));
        CHECK(mock->directPosts == 1);  // один прямой запрос
        CHECK(mock->storageGets >= 1);  // тянули S3-список
        CHECK(mock->healthGets >= 1);   // health-check прокси
        CHECK(mock->proxyPosts == 1);   // успешный POST через прокси
    }

    // --- post2: рабочий прокси закеширован → ни S3, ни health не нужны ------
    {
        const int storageBefore = mock->storageGets;
        const int healthBefore = mock->healthGets;
        Response r = client.post(endpoint, payload, ctx);
        CHECK(r.error == ErrorCode::NoError);
        CHECK_EQ(r.body, std::string(R"({"ok":true,"via":"proxy"})"));
        // прямой запрос ушёл сразу на кешированный прокси — без нового S3/health
        CHECK(mock->storageGets == storageBefore);
        CHECK(mock->healthGets == healthBefore);
        CHECK(mock->directPosts == 1);  // на шлюз больше не ходили
    }

    return AGW_TEST_MAIN_RETURN();
}
