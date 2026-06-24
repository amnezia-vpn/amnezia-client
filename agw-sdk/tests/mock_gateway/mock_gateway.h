#ifndef AGW_TEST_MOCK_GATEWAY_H
#define AGW_TEST_MOCK_GATEWAY_H

// In-process «шлюз»: расшифровывает запрос тестовой RSA-парой, как сервер, и шифрует ответ тем же
// AES-ключом/IV. Позволяет гонять весь конвейер post() без сети.

#include <string>
#include <vector>

#include "agw/http.h"
#include "crypto/aes.h"
#include "crypto/rsa.h"
#include "protocol/keys.h"
#include "util/base64.h"
#include "util/json.h"

namespace agw_test {

class MockGateway : public agw::IHttpClient {
public:
    explicit MockGateway(std::string privateKeyPem) : m_priv(std::move(privateKeyPem)) {}

    // Что вернуть как тело ответа (будет зашифровано тем же key/iv). По умолчанию — успех.
    std::string responsePlain = "{\"ok\":true}";
    bool simulateSsl = false;
    agw::TransportError simulateTransport = agw::TransportError::None;
    int httpStatusCode = 200;

    // Захваченное (для проверок в тестах).
    std::string lastUrl;
    std::string lastRequestId;
    std::string lastDecryptedPayload;
    int requestCount = 0;

    agw::HttpResponse send(const agw::HttpRequest &req) override
    {
        ++requestCount;
        lastUrl = req.url;
        for (const auto &h : req.headers) {
            if (h.first == "X-Client-Request-ID") {
                lastRequestId = h.second;
            }
        }

        agw::HttpResponse resp;
        resp.httpStatusCode = httpStatusCode;

        if (simulateSsl) {
            resp.sslError = true;
            resp.error = agw::TransportError::ConnectionError;
            return resp;
        }
        if (simulateTransport != agw::TransportError::None) {
            resp.error = simulateTransport;
            return resp;
        }

        namespace k = agw::protocol::keys;
        agw::util::Json body = agw::util::Json::parse(req.body);

        // ключи: base64 → RSA-decrypt → json_keys → aes_key/aes_iv
        const auto keyCipher = agw::util::base64Decode(body[k::keyPayload].get<std::string>());
        const auto keysBytes = agw::crypto::rsaDecryptPrivatePkcs1(keyCipher, m_priv);
        agw::util::Json keysJson = agw::util::Json::parse(std::string(keysBytes.begin(), keysBytes.end()));
        const auto aesKey = agw::util::base64Decode(keysJson[k::aesKey].get<std::string>());
        const auto aesIv = agw::util::base64Decode(keysJson[k::aesIv].get<std::string>());

        // payload: base64 → AES-decrypt
        const auto apiCipher = agw::util::base64Decode(body[k::apiPayload].get<std::string>());
        const auto payloadBytes = agw::crypto::aesDecryptCbc(apiCipher, aesKey, aesIv);
        lastDecryptedPayload.assign(payloadBytes.begin(), payloadBytes.end());

        // ответ: сырые байты AES(responsePlain) тем же key/iv (как настоящий шлюз)
        const std::vector<std::uint8_t> respPlain(responsePlain.begin(), responsePlain.end());
        const auto respCipher = agw::crypto::aesEncryptCbc(respPlain, aesKey, aesIv);
        resp.body.assign(respCipher.begin(), respCipher.end());
        resp.error = agw::TransportError::None;
        return resp;
    }

private:
    std::string m_priv;
};

} // namespace agw_test

#endif // AGW_TEST_MOCK_GATEWAY_H
