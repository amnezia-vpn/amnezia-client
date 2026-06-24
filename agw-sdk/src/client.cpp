#include "agw/client.h"

#include <memory>
#include <utility>

#include "crypto/rng.h"
#include "protocol/error_mapping.h"
#include "protocol/request_builder.h"
#include "protocol/response.h"
#include "util/url.h"
#include "util/uuid.h"

namespace agw {

struct GatewayClient::Impl {
    Config config;
    std::shared_ptr<IHttpClient> http;
    std::unique_ptr<crypto::IRng> rng;

    explicit Impl(Config cfg)
        : config(std::move(cfg)), rng(std::make_unique<crypto::DefaultRng>())
    {
        http = config.httpClient ? config.httpClient
                                  : std::shared_ptr<IHttpClient>(makeDefaultHttpClient());
    }

    void log(LogLevel level, const std::string &message) const
    {
        if (config.log) {
            config.log(level, message);
        }
    }

    // Ядро. Фаза 2 — только прямой запрос; failover (Фаза 3) встроится перед расшифровкой/маппингом.
    Response executePost(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx)
    {
        (void)ctx; // используется в failover (Фаза 3)

        protocol::EncryptedRequest enc =
            protocol::buildEncryptedRequest(payload, config.agwPublicKeyPem, *rng);
        if (enc.error != ErrorCode::NoError) {
            return Response{enc.error, std::string()};
        }

        const std::string requestId = util::makeUuidV4(*rng);
        const std::string url = util::formatEndpoint(endpoint, config.gatewayEndpoint);

        // Хук — один раз, с исходным хостом (паритет; прокси не вайтлистятся).
        if (config.onBeforeRequest) {
            config.onBeforeRequest(util::extractHost(url));
        }

        HttpRequest req;
        req.url = url;
        req.method = "POST";
        req.body = enc.body;
        req.headers = {
            {"Content-Type", "application/json"},
            {"X-Client-Request-ID", requestId},
        };
        req.timeoutMsecs = config.requestTimeoutMsecs;

        const HttpResponse resp = http->send(req);

        const protocol::DecryptResult dec = protocol::tryDecryptResponse(resp.body, enc.key, enc.iv);

        Response out;
        out.body = dec.decryptedBody;

        const ErrorCode mapped = protocol::mapResponseError(resp.sslError, resp.error, dec.decryptedBody);
        if (mapped != ErrorCode::NoError) {
            out.error = mapped;
            return out;
        }
        if (!dec.ok) {
            log(LogLevel::Error, "response decryption failed");
            out.error = ErrorCode::ApiConfigDecryptionError;
            return out;
        }
        out.error = ErrorCode::NoError;
        return out;
    }
};

GatewayClient::GatewayClient(Config config) : m_impl(std::make_unique<Impl>(std::move(config))) {}
GatewayClient::~GatewayClient() = default;
GatewayClient::GatewayClient(GatewayClient &&) noexcept = default;
GatewayClient &GatewayClient::operator=(GatewayClient &&) noexcept = default;

Response GatewayClient::post(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx)
{
    return m_impl->executePost(endpoint, payload, ctx);
}

} // namespace agw
