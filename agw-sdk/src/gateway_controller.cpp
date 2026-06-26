#include "agw/gateway_controller.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "crypto/rng.h"
#include "failover/bypass_policy.h"
#include "failover/proxy_list.h"
#include "failover/proxy_picker.h"
#include "protocol/error_mapping.h"
#include "protocol/request_builder.h"
#include "protocol/response.h"
#include "util/thread_pool.h"
#include "util/url.h"
#include "util/uuid.h"

namespace agw {
namespace {
bool isCancelled(const CancellationToken *cancel)
{
    return cancel != nullptr && cancel->isCancelled();
}

std::function<bool()> makeCancelCheck(CancellationToken *cancel)
{
    if (cancel == nullptr) {
        return {};
    }
    return [cancel] { return cancel->isCancelled(); };
}

std::string threadIdStr()
{
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

const char *transportErrorName(TransportError e)
{
    switch (e) {
    case TransportError::None: return "None";
    case TransportError::Timeout: return "Timeout";
    case TransportError::Canceled: return "Canceled";
    case TransportError::OperationNotImplemented: return "OperationNotImplemented";
    case TransportError::ConnectionError: return "ConnectionError";
    }
    return "?";
}
}

struct GatewayController::Impl {
    Config config;
    std::shared_ptr<IHttpClient> http;
    std::unique_ptr<crypto::IRng> rng;

    std::mutex proxyMutex;
    std::string cachedProxy;

    util::ThreadPool pool;

    explicit Impl(Config cfg)
        : config(std::move(cfg)),
          rng(std::make_unique<crypto::DefaultRng>()),
          pool(static_cast<std::size_t>(config.threadPoolSize))
    {
        http = config.httpClient ? config.httpClient
                                  : std::shared_ptr<IHttpClient>(makeDefaultHttpClient());
        log(LogLevel::Info,
            "client created: dev=" + std::string(config.isDevEnvironment ? "1" : "0")
                + " timeout=" + std::to_string(config.requestTimeoutMsecs) + "ms"
                + " pool=" + std::to_string(config.threadPoolSize)
                + " s3primary=" + std::to_string(config.s3PrimaryEndpoints.size())
                + " s3fallback=" + std::to_string(config.s3FallbackEndpoints.size())
                + " customHttp=" + std::string(config.httpClient ? "1" : "0"));
    }

    void log(LogLevel level, const std::string &message) const
    {
        if (config.log) {
            config.log(level, message);
        }
    }
    void dbg(const std::string &message) const { log(LogLevel::Debug, message); }

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

    bool attempt(const std::string &endpoint, const std::string &host, const HttpRequest &baseReq,
                 const std::vector<std::uint8_t> &key, const std::vector<std::uint8_t> &iv,
                 HttpResponse &resp, protocol::DecryptResult &dec)
    {
        HttpRequest req = baseReq;
        req.url = util::formatEndpoint(endpoint, host);
        dbg("  proxy attempt: POST " + req.url);

        const auto t0 = std::chrono::steady_clock::now();
        resp = http->send(req);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now() - t0).count();

        dec = protocol::tryDecryptResponse(resp.body, key, iv);
        const bool bypass = resp.sslError || failover::shouldBypassProxy(resp.error, dec.decryptedBody, dec.ok);
        dbg("  proxy attempt result: transport=" + std::string(transportErrorName(resp.error))
            + " ssl=" + std::string(resp.sslError ? "1" : "0") + " http=" + std::to_string(resp.httpStatusCode)
            + " bodyLen=" + std::to_string(resp.body.size()) + " decryptOk=" + std::string(dec.ok ? "1" : "0")
            + " bypassAgain=" + std::string(bypass ? "1" : "0") + " (" + std::to_string(ms) + "ms)");
        return !bypass;
    }

    void runFailover(const std::string &endpoint, const HttpRequest &baseReq, const FailoverContext &ctx,
                     const std::vector<std::uint8_t> &key, const std::vector<std::uint8_t> &iv,
                     HttpResponse &resp, protocol::DecryptResult &dec, CancellationToken *cancel)
    {
        if (isCancelled(cancel)) {
            dbg("failover: cancelled before start");
            return;
        }

        std::random_device rd;
        std::mt19937 gen(rd());

        std::vector<std::string> primary = config.s3PrimaryEndpoints;
        std::vector<std::string> fallback = config.s3FallbackEndpoints;
        std::shuffle(primary.begin(), primary.end(), gen);
        std::shuffle(fallback.begin(), fallback.end(), gen);

        const std::vector<std::string> storageUrls = failover::buildStorageUrls(primary, fallback, ctx);
        dbg("failover: storage urls=" + std::to_string(storageUrls.size())
            + " service='" + ctx.serviceType + "' country='" + ctx.userCountryCode + "'");

        std::vector<std::string> proxyUrls;
        for (const auto &storageUrl : storageUrls) {
            if (isCancelled(cancel)) {
                dbg("failover: cancelled during storage fetch");
                return;
            }
            HttpRequest g;
            g.url = storageUrl;
            g.method = "GET";
            g.headers = {{"Content-Type", "application/json"}};
            g.timeoutMsecs = config.proxyStorageTimeoutMsecs;
            g.cancelCheck = makeCancelCheck(cancel);

            const HttpResponse gr = http->send(g);
            dbg("  storage GET " + storageUrl + " → transport=" + std::string(transportErrorName(gr.error))
                + " ssl=" + std::string(gr.sslError ? "1" : "0") + " http=" + std::to_string(gr.httpStatusCode)
                + " bodyLen=" + std::to_string(gr.body.size()));
            if (gr.error != TransportError::None || gr.sslError) {
                continue;
            }
            try {
                proxyUrls = failover::decodeProxyList(gr.body, config.isDevEnvironment, config.agwPublicKeyPem);
                dbg("  decoded proxy list: " + std::to_string(proxyUrls.size()) + " proxies");
                break;
            } catch (...) {
                dbg("  proxy list decode failed → next storage");
                continue;
            }
        }

        std::shuffle(proxyUrls.begin(), proxyUrls.end(), gen);

        std::string proxy = getCachedProxy();
        if (proxy.empty()) {
            if (isCancelled(cancel)) {
                dbg("failover: cancelled before health-check");
                return;
            }
            dbg("failover: no cached proxy → health-check of " + std::to_string(proxyUrls.size()) + " proxies");
            proxy = failover::pickHealthyProxy(*http, proxyUrls, config.proxyHealthTimeoutMsecs);
            if (!proxy.empty()) {
                dbg("failover: healthy proxy = " + proxy + " (cached)");
                setCachedProxy(proxy);
            } else {
                dbg("failover: no healthy proxy found");
            }
        } else {
            dbg("failover: using cached proxy = " + proxy);
        }

        if (!proxy.empty()) {
            if (isCancelled(cancel)) {
                return;
            }
            if (attempt(endpoint, proxy, baseReq, key, iv, resp, dec)) {
                dbg("failover: succeeded via cached/first proxy");
                return;
            }
        }
        for (const auto &p : proxyUrls) {
            if (isCancelled(cancel)) {
                return;
            }
            if (attempt(endpoint, p, baseReq, key, iv, resp, dec)) {
                dbg("failover: succeeded via proxy " + p + " (cached)");
                setCachedProxy(p);
                return;
            }
        }
        dbg("failover: exhausted all proxies (using last attempt result)");
    }

    Response executePost(const std::string &endpoint, const std::string &payload,
                         const FailoverContext &ctx, CancellationToken *cancel)
    {
        const auto tStart = std::chrono::steady_clock::now();
        log(LogLevel::Info, "post START endpoint='" + endpoint + "' service='" + ctx.serviceType
            + "' country='" + ctx.userCountryCode + "' payloadLen=" + std::to_string(payload.size())
            + " thread=" + threadIdStr());

        if (isCancelled(cancel)) {
            log(LogLevel::Info, "post: cancelled before start");
            return Response{ErrorCode::Cancelled, std::string()};
        }

        protocol::EncryptedRequest enc =
            protocol::buildEncryptedRequest(payload, config.agwPublicKeyPem, *rng);
        if (enc.error != ErrorCode::NoError) {
            log(LogLevel::Warning, "post: request build failed error="
                + std::to_string(static_cast<int>(enc.error)));
            return Response{enc.error, std::string()};
        }
        dbg("request built: bodyLen=" + std::to_string(enc.body.size()) + " (key/iv/salt generated)");
        if (isCancelled(cancel)) {
            return Response{ErrorCode::Cancelled, std::string()};
        }

        const std::string requestId = util::makeUuidV4(*rng);
        const std::string cached = getCachedProxy();
        const std::string directHost = cached.empty() ? config.gatewayEndpoint : cached;
        const std::string url = util::formatEndpoint(endpoint, directHost);
        dbg("direct request: url=" + url + " reqId=" + requestId
            + " viaCachedProxy=" + std::string(cached.empty() ? "0" : "1"));

        if (config.onBeforeRequest) {
            const std::string host = util::extractHost(url);
            dbg("onBeforeRequest(host=" + host + ")");
            config.onBeforeRequest(host);
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
        req.cancelCheck = makeCancelCheck(cancel);

        const auto t0 = std::chrono::steady_clock::now();
        HttpResponse resp = http->send(req);
        const auto httpMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::steady_clock::now() - t0).count();
        if (isCancelled(cancel)) {
            log(LogLevel::Info, "post: cancelled after direct send");
            return Response{ErrorCode::Cancelled, std::string()};
        }

        protocol::DecryptResult dec = protocol::tryDecryptResponse(resp.body, enc.key, enc.iv);
        dbg("direct response: transport=" + std::string(transportErrorName(resp.error))
            + " ssl=" + std::string(resp.sslError ? "1" : "0") + " http=" + std::to_string(resp.httpStatusCode)
            + " bodyLen=" + std::to_string(resp.body.size()) + " decryptOk=" + std::string(dec.ok ? "1" : "0")
            + " (" + std::to_string(httpMs) + "ms)");

        const bool bypass = !resp.sslError
            && failover::shouldBypassProxy(resp.error, dec.decryptedBody, dec.ok);
        if (bypass) {
            log(LogLevel::Info, "direct response suspicious — running failover");
            runFailover(endpoint, req, ctx, enc.key, enc.iv, resp, dec, cancel);
            if (isCancelled(cancel)) {
                log(LogLevel::Info, "post: cancelled during failover");
                return Response{ErrorCode::Cancelled, std::string()};
            }
        } else {
            dbg("direct response accepted (no failover)");
        }

        Response out;
        out.body = dec.decryptedBody;

        const ErrorCode mapped = protocol::mapResponseError(resp.sslError, resp.error, dec.decryptedBody);
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                                 std::chrono::steady_clock::now() - tStart).count();
        if (mapped != ErrorCode::NoError) {
            out.error = mapped;
            log(LogLevel::Warning, "post DONE error=" + std::to_string(static_cast<int>(mapped))
                + " bodyLen=" + std::to_string(out.body.size()) + " (" + std::to_string(totalMs) + "ms)");
            return out;
        }
        if (!dec.ok) {
            out.error = ErrorCode::ApiConfigDecryptionError;
            log(LogLevel::Error, "post DONE: response decryption failed (1106) ("
                + std::to_string(totalMs) + "ms)");
            return out;
        }
        out.error = ErrorCode::NoError;
        log(LogLevel::Info, "post DONE ok bodyLen=" + std::to_string(out.body.size())
            + " (" + std::to_string(totalMs) + "ms)");
        return out;
    }
};

GatewayController::GatewayController(Config config) : m_impl(std::make_unique<Impl>(std::move(config))) {}
GatewayController::~GatewayController() = default;
GatewayController::GatewayController(GatewayController &&) noexcept = default;
GatewayController &GatewayController::operator=(GatewayController &&) noexcept = default;

Response GatewayController::post(const std::string &endpoint, const std::string &payload,
                             const FailoverContext &ctx, CancellationToken *cancel)
{
    return m_impl->executePost(endpoint, payload, ctx, cancel);
}

void GatewayController::postAsync(const std::string &endpoint, const std::string &payload,
                              std::function<void(Response)> onResult, const FailoverContext &ctx,
                              CancellationToken *cancel)
{
    Impl *impl = m_impl.get();
    impl->dbg("postAsync: submitting to pool (caller thread=" + threadIdStr() + ")");
    impl->pool.submit([impl, endpoint, payload, onResult = std::move(onResult), ctx, cancel]() {
        impl->dbg("postAsync: running on pool thread=" + threadIdStr());
        Response r = impl->executePost(endpoint, payload, ctx, cancel);
        if (onResult) {
            onResult(std::move(r));
        }
    });
}

std::future<Response> GatewayController::postFuture(const std::string &endpoint, const std::string &payload,
                                                const FailoverContext &ctx, CancellationToken *cancel)
{
    auto promise = std::make_shared<std::promise<Response>>();
    std::future<Response> fut = promise->get_future();
    Impl *impl = m_impl.get();
    impl->dbg("postFuture: submitting to pool (caller thread=" + threadIdStr() + ")");
    impl->pool.submit([impl, endpoint, payload, ctx, cancel, promise]() {
        impl->dbg("postFuture: running on pool thread=" + threadIdStr());
        promise->set_value(impl->executePost(endpoint, payload, ctx, cancel));
    });
    return fut;
}
}
