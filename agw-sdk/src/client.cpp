#include "agw/client.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "crypto/rng.h"
#include "failover/bypass_policy.h"
#include "failover/proxy_list.h"
#include "failover/proxy_picker.h"
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

    std::mutex proxyMutex;
    std::string cachedProxy;  // рабочий прокси, переживает между запросами (бывш. static m_proxyUrl)

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

    std::string getCachedProxy()
    {
        std::lock_guard<std::mutex> lock(proxyMutex);
        return cachedProxy;
    }

    void setCachedProxy(const std::string &proxy)
    {
        std::lock_guard<std::mutex> lock(proxyMutex);
        cachedProxy = proxy;
    }

    // Один POST через указанный хост. Обновляет resp/dec последней попытки. Возвращает true,
    // если ответ «хороший» (нет ssl-ошибки и не нужен дальнейший байпас).
    bool attempt(const std::string &endpoint, const std::string &host, const HttpRequest &baseReq,
                 const std::vector<std::uint8_t> &key, const std::vector<std::uint8_t> &iv,
                 HttpResponse &resp, protocol::DecryptResult &dec)
    {
        HttpRequest req = baseReq;  // то же тело, тот же X-Client-Request-ID, те же заголовки
        req.url = util::formatEndpoint(endpoint, host);
        resp = http->send(req);
        dec = protocol::tryDecryptResponse(resp.body, key, iv);
        if (resp.sslError || failover::shouldBypassProxy(resp.error, dec.decryptedBody, dec.ok)) {
            return false;
        }
        return true;
    }

    // Обход блокировок: тянет список прокси из S3, health-check, перебор. Обновляет resp/dec на
    // последнюю попытку; если попыток не было — оставляет прямой ответ (паритет с bypassProxy).
    void runFailover(const std::string &endpoint, const HttpRequest &baseReq, const FailoverContext &ctx,
                     const std::vector<std::uint8_t> &key, const std::vector<std::uint8_t> &iv,
                     HttpResponse &resp, protocol::DecryptResult &dec)
    {
        std::random_device rd;
        std::mt19937 gen(rd());

        std::vector<std::string> primary = config.s3PrimaryEndpoints;
        std::vector<std::string> fallback = config.s3FallbackEndpoints;
        std::shuffle(primary.begin(), primary.end(), gen);
        std::shuffle(fallback.begin(), fallback.end(), gen);

        const std::vector<std::string> storageUrls = failover::buildStorageUrls(primary, fallback, ctx);

        std::vector<std::string> proxyUrls;
        for (const auto &storageUrl : storageUrls) {
            HttpRequest g;
            g.url = storageUrl;
            g.method = "GET";
            g.headers = {{"Content-Type", "application/json"}};
            g.timeoutMsecs = config.proxyStorageTimeoutMsecs;

            const HttpResponse gr = http->send(g);
            if (gr.error != TransportError::None || gr.sslError) {
                continue;
            }
            try {
                proxyUrls = failover::decodeProxyList(gr.body, config.isDevEnvironment, config.agwPublicKeyPem);
                break;  // первый успешно прочитанный список — как в оригинале (даже если пуст)
            } catch (...) {
                continue;  // сбой расшифровки → следующее хранилище
            }
        }

        std::shuffle(proxyUrls.begin(), proxyUrls.end(), gen);

        // кеш пуст → health-check выбирает первый живой прокси в кеш
        std::string proxy = getCachedProxy();
        if (proxy.empty()) {
            proxy = failover::pickHealthyProxy(*http, proxyUrls, config.proxyHealthTimeoutMsecs);
            if (!proxy.empty()) {
                setCachedProxy(proxy);
            }
        }

        if (!proxy.empty()) {
            if (attempt(endpoint, proxy, baseReq, key, iv, resp, dec)) {
                return;
            }
        }
        for (const auto &p : proxyUrls) {
            if (attempt(endpoint, p, baseReq, key, iv, resp, dec)) {
                setCachedProxy(p);
                return;
            }
        }
    }

    Response executePost(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx)
    {
        protocol::EncryptedRequest enc =
            protocol::buildEncryptedRequest(payload, config.agwPublicKeyPem, *rng);
        if (enc.error != ErrorCode::NoError) {
            return Response{enc.error, std::string()};
        }

        const std::string requestId = util::makeUuidV4(*rng);
        // Прямой запрос идёт на кешированный прокси, если он есть (паритет: prepareRequest
        // ставит url = proxy.isEmpty() ? gateway : proxy).
        const std::string cached = getCachedProxy();
        const std::string directHost = cached.empty() ? config.gatewayEndpoint : cached;
        const std::string url = util::formatEndpoint(endpoint, directHost);

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

        HttpResponse resp = http->send(req);
        protocol::DecryptResult dec = protocol::tryDecryptResponse(resp.body, enc.key, enc.iv);

        if (!resp.sslError && failover::shouldBypassProxy(resp.error, dec.decryptedBody, dec.ok)) {
            log(LogLevel::Info, "direct response suspicious — running failover");
            runFailover(endpoint, req, ctx, enc.key, enc.iv, resp, dec);
        }

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
