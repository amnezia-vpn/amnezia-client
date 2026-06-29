#include "agw/c_abi.h"

#include <cstdlib>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "agw/cancellation.h"
#include "agw/gateway_controller.h"
#include "agw/config.h"
#include "detail/test_hooks.h"
#include "services/api_methods.h"
#include "util/json.h"

struct agw_client
{
    explicit agw_client(agw::Config cfg) : client(std::move(cfg))
    {
    }
    agw::GatewayController client;
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
    }

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
}

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

    // Копия std::string в malloc'нутый char* (+'\0'). Пустую — в NULL (необяз. поля). free(NULL) безопасен.
    char *dupStr(const std::string &s)
    {
        if (s.empty()) {
            return nullptr;
        }
        char *buf = static_cast<char *>(std::malloc(s.size() + 1));
        if (buf != nullptr) {
            std::memcpy(buf, s.data(), s.size());
            buf[s.size()] = '\0';
        }
        return buf;
    }

    agw::services::GatewayRequest toGatewayRequest(const agw_gateway_request *r)
    {
        agw::services::GatewayRequest out;
        if (r == nullptr) {
            return out;
        }
        out.osVersion = cstr(r->os_version);
        out.appVersion = cstr(r->app_version);
        out.appLanguage = cstr(r->app_language);
        out.installationUuid = cstr(r->installation_uuid);
        out.userCountryCode = cstr(r->user_country_code);
        out.serverCountryCode = cstr(r->server_country_code);
        out.serviceType = cstr(r->service_type);
        out.serviceProtocol = cstr(r->service_protocol);
        const std::string auth = cstr(r->auth_data_json);
        if (!auth.empty()) {
            try {
                out.authData = agw::util::Json::parse(auth);
            } catch (...) {
            }
        }
        return out;
    }

    agw::services::AccountRequest toAccountRequest(const agw_gateway_request *r, const char *cliVersion,
                                                   const char *subscriptionStatus)
    {
        agw::services::AccountRequest out;
        out.base = toGatewayRequest(r);
        out.cliVersion = cstr(cliVersion);
        out.subscriptionStatus = cstr(subscriptionStatus);
        return out;
    }

    void fillApiConfig(agw_api_config &out, const agw::ApiConfig &c)
    {
        out.service_type = dupStr(c.serviceType);
        out.service_protocol = dupStr(c.serviceProtocol);
        out.user_country_code = dupStr(c.userCountryCode);
        out.server_country_code = dupStr(c.serverCountryCode);
        out.server_country_name = dupStr(c.serverCountryName);
        out.vpn_key = dupStr(c.vpnKey);
        out.subscription_end_date = dupStr(c.subscriptionEndDate);
        out.active_device_count = c.activeDeviceCount;
        out.max_device_count = c.maxDeviceCount;
        out.issued_configs = c.issuedConfigs;
        out.available_countries_json = dupStr(c.availableCountriesJson);
        out.supported_protocols_json = dupStr(c.supportedProtocolsJson);
        out.is_ad_visible = c.serviceInfo.isAdVisible ? 1 : 0;
        out.is_renewal_available = c.serviceInfo.isRenewalAvailable ? 1 : 0;
        out.ad_header = dupStr(c.serviceInfo.adHeader);
        out.ad_description = dupStr(c.serviceInfo.adDescription);
        out.ad_endpoint = dupStr(c.serviceInfo.adEndpoint);
        out.public_key_expires_at = dupStr(c.publicKeyExpiresAt);
        out.stack_type = dupStr(c.stackType);
        out.cli_version = dupStr(c.cliVersion);
        out.is_test_purchase = c.isTestPurchase ? 1 : 0;
        out.is_in_app_purchase = c.isInAppPurchase ? 1 : 0;
        out.subscription_expired_by_server = c.subscriptionExpiredByServer ? 1 : 0;
    }

    agw_json_result toJsonResult(const agw::services::JsonResult &r)
    {
        agw_json_result out{};
        out.error = static_cast<int>(r.error);
        const std::string dumped = r.value.is_null() ? std::string() : r.value.dump();
        out.json = dupStr(dumped);
        out.json_len = dumped.size();
        return out;
    }

    agw_import_result toImportResult(const agw::services::ImportResult &r)
    {
        agw_import_result out{};
        out.error = static_cast<int>(r.error);
        out.captcha_required = r.captchaRequired ? 1 : 0;
        out.captcha_id = dupStr(r.captcha.captchaId);
        out.captcha_image = dupStr(r.captcha.captchaImageBase64);
        out.captcha_hint = dupStr(r.captcha.hint);
        out.server_config_json = dupStr(r.serverConfigJson);
        out.raw_response_json = dupStr(r.rawResponseJson);
        return out;
    }
}

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
                agw_response cr = toCResponse(r);
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

agw_json_result agw_get_services(agw_client *client, const char *os_version, const char *app_version,
                                 const char *cli_name, const char *app_language)
{
    if (client == nullptr) {
        agw_json_result out{};
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    agw::services::ServicesRequest req;
    req.osVersion = cstr(os_version);
    req.appVersion = cstr(app_version);
    req.cliName = cstr(cli_name);
    req.appLanguage = cstr(app_language);
    return toJsonResult(agw::services::getServices(client->client, req));
}

agw_json_result agw_get_news(agw_client *client, const char *locale, const char *const *country_codes,
                             size_t country_count, const char *const *service_types, size_t service_count)
{
    if (client == nullptr) {
        agw_json_result out{};
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    agw::services::NewsRequest req;
    req.locale = cstr(locale);
    for (size_t i = 0; i < country_count && country_codes != nullptr; ++i) {
        req.userCountryCodes.push_back(cstr(country_codes[i]));
    }
    for (size_t i = 0; i < service_count && service_types != nullptr; ++i) {
        req.serviceTypes.push_back(cstr(service_types[i]));
    }
    return toJsonResult(agw::services::getNews(client->client, req));
}

agw_url_result agw_get_updater_endpoint(agw_client *client, const char *cli_version, const char *os_version,
                                        const char *installation_uuid)
{
    agw_url_result out{};
    if (client == nullptr) {
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    agw::services::UpdaterRequest req;
    req.cliVersion = cstr(cli_version);
    req.osVersion = cstr(os_version);
    req.installationUuid = cstr(installation_uuid);
    const agw::services::UpdateResult r = agw::services::getUpdaterEndpoint(client->client, req);
    out.error = static_cast<int>(r.error);
    out.url = dupStr(r.url);
    out.raw_json = dupStr(r.raw.is_null() ? std::string() : r.raw.dump());
    return out;
}

agw_account_info_result agw_get_account_info(agw_client *client, const agw_gateway_request *req,
                                             const char *cli_version, const char *subscription_status)
{
    agw_account_info_result out{};
    if (client == nullptr) {
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    const agw::services::AccountInfoResult r =
            agw::services::getAccountInfo(client->client, toAccountRequest(req, cli_version, subscription_status));
    out.error = static_cast<int>(r.error);
    fillApiConfig(out.account, r.account);
    return out;
}

agw_url_result agw_get_renewal_link(agw_client *client, const agw_gateway_request *req, const char *cli_version,
                                    const char *subscription_status)
{
    agw_url_result out{};
    if (client == nullptr) {
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    const agw::services::RenewalResult r =
            agw::services::getRenewalLink(client->client, toAccountRequest(req, cli_version, subscription_status));
    out.error = static_cast<int>(r.error);
    out.url = dupStr(r.renewalUrl);
    out.raw_json = nullptr;
    return out;
}

agw_import_result agw_import_service(agw_client *client, const agw_gateway_request *req, const char *public_key)
{
    if (client == nullptr) {
        agw_import_result out{};
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    return toImportResult(agw::services::importService(client->client, toGatewayRequest(req), cstr(public_key)));
}

agw_import_result agw_import_trial(agw_client *client, const agw_gateway_request *req, const char *public_key,
                                   const char *email)
{
    if (client == nullptr) {
        agw_import_result out{};
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    return toImportResult(
            agw::services::importTrial(client->client, toGatewayRequest(req), cstr(public_key), cstr(email)));
}

agw_import_result agw_resolve_import_captcha(agw_client *client, const agw_gateway_request *req,
                                             const char *public_key, const char *captcha_id,
                                             const char *captcha_solution)
{
    if (client == nullptr) {
        agw_import_result out{};
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    return toImportResult(agw::services::resolveImportCaptcha(client->client, toGatewayRequest(req), cstr(public_key),
                                                              cstr(captcha_id), cstr(captcha_solution)));
}

int agw_deactivate_device(agw_client *client, const agw_gateway_request *req)
{
    if (client == nullptr) {
        return static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
    }
    return static_cast<int>(agw::services::deactivateDevice(client->client, toGatewayRequest(req)));
}

agw_app_store_result agw_import_from_app_store(agw_client *client, const agw_gateway_request *req,
                                               const char *public_key, const char *transaction_id)
{
    agw_app_store_result out{};
    if (client == nullptr) {
        out.error = static_cast<int>(agw::ErrorCode::ApiConfigDownloadError);
        return out;
    }
    const agw::services::AppStoreImportResult r = agw::services::importServiceFromAppStore(
            client->client, toGatewayRequest(req), cstr(public_key), cstr(transaction_id));
    out.error = static_cast<int>(r.error);
    out.server_config_json = dupStr(r.serverConfigJson);
    out.vpn_key = dupStr(r.vpnKey);
    out.crc = r.crc;
    return out;
}

void agw_json_result_free(agw_json_result *result)
{
    if (result == nullptr) {
        return;
    }
    std::free(result->json);
    result->json = nullptr;
    result->json_len = 0;
}

void agw_url_result_free(agw_url_result *result)
{
    if (result == nullptr) {
        return;
    }
    std::free(result->url);
    std::free(result->raw_json);
    result->url = nullptr;
    result->raw_json = nullptr;
}

void agw_account_info_result_free(agw_account_info_result *result)
{
    if (result == nullptr) {
        return;
    }
    agw_api_config &c = result->account;
    char *fields[] = { c.service_type,    c.service_protocol,         c.user_country_code,
                       c.server_country_code, c.server_country_name,   c.vpn_key,
                       c.subscription_end_date, c.available_countries_json, c.supported_protocols_json,
                       c.ad_header,        c.ad_description,           c.ad_endpoint,
                       c.public_key_expires_at, c.stack_type,          c.cli_version };
    for (char *f : fields) {
        std::free(f);
    }
    c = agw_api_config{};
}

void agw_import_result_free(agw_import_result *result)
{
    if (result == nullptr) {
        return;
    }
    std::free(result->captcha_id);
    std::free(result->captcha_image);
    std::free(result->captcha_hint);
    std::free(result->server_config_json);
    std::free(result->raw_response_json);
    result->captcha_id = nullptr;
    result->captcha_image = nullptr;
    result->captcha_hint = nullptr;
    result->server_config_json = nullptr;
    result->raw_response_json = nullptr;
}

void agw_app_store_result_free(agw_app_store_result *result)
{
    if (result == nullptr) {
        return;
    }
    std::free(result->server_config_json);
    std::free(result->vpn_key);
    result->server_config_json = nullptr;
    result->vpn_key = nullptr;
}
}
