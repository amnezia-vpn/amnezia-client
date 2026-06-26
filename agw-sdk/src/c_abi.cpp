#include "agw/c_abi.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "agw/cancellation.h"
#include "agw/client.h"
#include "agw/config.h"
#include "detail/test_hooks.h"

struct agw_client
{
    explicit agw_client(agw::Config cfg) : client(std::move(cfg))
    {
    }
    agw::GatewayClient client;
};

struct agw_cancel_token
{
    agw::CancellationToken token;
};

namespace agw::detail
{

    namespace
    {
        std::mutex g_testHttpMutex;
        std::shared_ptr<IHttpClient> g_testHttp;
    } // namespace

    void setNextTestHttpClient(std::shared_ptr<IHttpClient> http)
    {
        std::lock_guard<std::mutex> lock(g_testHttpMutex);
        g_testHttp = std::move(http);
    }

    std::shared_ptr<IHttpClient> takeNextTestHttpClient()
    {
        std::lock_guard<std::mutex> lock(g_testHttpMutex);
        std::shared_ptr<IHttpClient> h = std::move(g_testHttp);
        g_testHttp.reset();
        return h;
    }

} // namespace agw::detail

namespace
{

    std::string cstr(const char *s)
    {
        return s ? std::string(s) : std::string();
    }

    agw_response toCResponse(const agw::Response &r)
    {
        agw_response out;
        out.error = static_cast<int>(r.error);
        out.body = nullptr;
        out.body_len = r.body.size();

        char *buf = static_cast<char *>(std::malloc(r.body.size() + 1));
        if (buf != nullptr) {
            if (!r.body.empty()) {
                std::memcpy(buf, r.body.data(), r.body.size());
            }
            buf[r.body.size()] = '\0';
            out.body = buf;
        } else {
            out.body_len = 0;
        }
        return out;
    }

} // namespace

extern "C" {

agw_client *agw_client_create(const agw_config *config)
{
    if (config == nullptr) {
        return nullptr;
    }

    agw::Config cfg;
    cfg.gatewayEndpoint = cstr(config->gateway_endpoint);
    cfg.agwPublicKeyPem = cstr(config->agw_public_key_pem);

    for (size_t i = 0; i < config->s3_primary_count; ++i) {
        cfg.s3PrimaryEndpoints.push_back(cstr(config->s3_primary_endpoints[i]));
    }
    for (size_t i = 0; i < config->s3_fallback_count; ++i) {
        cfg.s3FallbackEndpoints.push_back(cstr(config->s3_fallback_endpoints[i]));
    }

    cfg.isDevEnvironment = config->is_dev_environment != 0;
    if (config->request_timeout_msecs > 0)
        cfg.requestTimeoutMsecs = config->request_timeout_msecs;
    if (config->proxy_health_timeout_msecs > 0)
        cfg.proxyHealthTimeoutMsecs = config->proxy_health_timeout_msecs;
    if (config->proxy_storage_timeout_msecs > 0)
        cfg.proxyStorageTimeoutMsecs = config->proxy_storage_timeout_msecs;
    if (config->thread_pool_size > 0)
        cfg.threadPoolSize = config->thread_pool_size;

    if (config->on_before_request != nullptr) {
        agw_before_request_fn fn = config->on_before_request;
        void *ud = config->on_before_request_user_data;
        cfg.onBeforeRequest = [fn, ud](const std::string &host) { fn(host.c_str(), ud); };
    }
    if (config->log != nullptr) {
        agw_log_fn fn = config->log;
        void *ud = config->log_user_data;
        cfg.log = [fn, ud](agw::LogLevel level, const std::string &msg) { fn(static_cast<int>(level), msg.c_str(), ud); };
    }

    if (auto http = agw::detail::takeNextTestHttpClient()) {
        cfg.httpClient = http;
    }

    try {
        return new agw_client(std::move(cfg));
    } catch (...) {
        return nullptr;
    }
}

void agw_client_destroy(agw_client *client)
{
    delete client;
}

agw_response agw_client_post(agw_client *client, const char *endpoint, const char *payload, const char *service_type,
                             const char *user_country_code, agw_cancel_token *cancel_token)
{
    if (client == nullptr) {
        agw_response out;
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        out.body = nullptr;
        out.body_len = 0;
        return out;
    }

    agw::FailoverContext ctx { cstr(service_type), cstr(user_country_code) };
    agw::CancellationToken *tk = cancel_token ? &cancel_token->token : nullptr;
    agw::Response r = client->client.post(cstr(endpoint), cstr(payload), ctx, tk);
    return toCResponse(r);
}

void agw_client_post_async(agw_client *client, const char *endpoint, const char *payload, const char *service_type,
                           const char *user_country_code, agw_post_callback callback, void *user_data,
                           agw_cancel_token *cancel_token)
{
    if (client == nullptr || callback == nullptr) {
        return;
    }

    agw::FailoverContext ctx { cstr(service_type), cstr(user_country_code) };
    agw::CancellationToken *tk = cancel_token ? &cancel_token->token : nullptr;

    client->client.postAsync(
            cstr(endpoint), cstr(payload),
            [callback, user_data](agw::Response r) {
                agw_response cr = toCResponse(r); // владение телом переходит коллбэку
                callback(cr, user_data);
            },
            ctx, tk);
}

void agw_response_free(agw_response *response)
{
    if (response == nullptr) {
        return;
    }
    std::free(response->body);
    response->body = nullptr;
    response->body_len = 0;
}

agw_cancel_token *agw_cancel_token_create(void)
{
    try {
        return new agw_cancel_token();
    } catch (...) {
        return nullptr;
    }
}

void agw_cancel_token_cancel(agw_cancel_token *token)
{
    if (token != nullptr) {
        token->token.cancel();
    }
}

void agw_cancel_token_destroy(agw_cancel_token *token)
{
    delete token;
}

} // extern "C"
