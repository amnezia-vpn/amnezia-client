#ifndef AGW_C_ABI_H
#define AGW_C_ABI_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
    #if defined(AGW_BUILDING_SHARED)
        #define AGW_API __declspec(dllexport)
    #elif defined(AGW_USING_SHARED)
        #define AGW_API __declspec(dllimport)
    #else
        #define AGW_API
    #endif
#else
    #define AGW_API __attribute__((visibility("default")))
#endif

typedef struct agw_client agw_client;
typedef struct agw_cancel_token agw_cancel_token;

typedef void (*agw_before_request_fn)(const char *host, void *user_data);
typedef void (*agw_log_fn)(int level, const char *message, void *user_data);

typedef struct
{
    const char *gateway_endpoint;
    const char *agw_public_key_pem;

    const char *const *s3_primary_endpoints;
    size_t s3_primary_count;
    const char *const *s3_fallback_endpoints;
    size_t s3_fallback_count;

    int is_dev_environment;
    int request_timeout_msecs;
    int proxy_health_timeout_msecs;
    int proxy_storage_timeout_msecs;
    int thread_pool_size;

    agw_before_request_fn on_before_request;
    void *on_before_request_user_data;
    agw_log_fn log;
    void *log_user_data;
} agw_config;

typedef struct
{
    int error;
    char *body;
    size_t body_len;
} agw_response;

typedef void (*agw_post_callback)(agw_response response, void *user_data);

AGW_API agw_client *agw_client_create(const agw_config *config);
AGW_API void agw_client_destroy(agw_client *client);

AGW_API agw_response agw_client_post(agw_client *client, const char *endpoint, const char *payload,
                                     const char *service_type, const char *user_country_code,
                                     agw_cancel_token *cancel_token);

AGW_API void agw_client_post_async(agw_client *client, const char *endpoint, const char *payload,
                                   const char *service_type, const char *user_country_code, agw_post_callback callback,
                                   void *user_data, agw_cancel_token *cancel_token);

AGW_API void agw_response_free(agw_response *response);

AGW_API agw_cancel_token *agw_cancel_token_create(void);
AGW_API void agw_cancel_token_cancel(agw_cancel_token *token);
AGW_API void agw_cancel_token_destroy(agw_cancel_token *token);

/* ===========================================================================
 * Tier 2 — типизированные методы (Шаг 5).
 * Все char* в результатах выделены в куче и принадлежат вызывающему: освобождать
 * соответствующим *_free. Входы (const char*) копируются внутри, владелец — вызывающий.
 * Test/Sandbox vs Prod, таймауты, ключ шлюза — в agw_config при создании клиента.
 * ========================================================================= */

/* Базовый gateway-запрос (паритет с GatewayRequestData::toJsonObject — пустые поля опускаются). */
typedef struct
{
    const char *os_version;
    const char *app_version;
    const char *app_language;
    const char *installation_uuid;
    const char *user_country_code;
    const char *server_country_code;
    const char *service_type;
    const char *service_protocol;
    const char *auth_data_json; /* опц. JSON-объект (напр. {"api_key":"..."}); NULL/"" — пропустить */
} agw_gateway_request;

/* Результат с одним JSON-телом (services-объект; news-массив). */
typedef struct
{
    int error;
    char *json;
    size_t json_len;
} agw_json_result;

/* Результат с URL (+ сырой JSON для updater_endpoint). */
typedef struct
{
    int error;
    char *url;
    char *raw_json; /* может быть NULL */
} agw_url_result;

/* Плоское зеркало agw::ApiConfig. */
typedef struct
{
    char *service_type;
    char *service_protocol;
    char *user_country_code;
    char *server_country_code;
    char *server_country_name;
    char *vpn_key;
    char *subscription_end_date;
    int active_device_count;
    int max_device_count;
    int issued_configs;
    char *available_countries_json;
    char *supported_protocols_json;
    int is_ad_visible;
    int is_renewal_available;
    char *ad_header;
    char *ad_description;
    char *ad_endpoint;
    char *public_key_expires_at;
    char *stack_type;
    char *cli_version;
    int is_test_purchase;
    int is_in_app_purchase;
    int subscription_expired_by_server;
} agw_api_config;

typedef struct
{
    int error;
    agw_api_config account;
} agw_account_info_result;

/* Результат import/trial/resolveCaptcha. captcha_required=1 → показать капчу и повторить через resolve. */
typedef struct
{
    int error;
    int captcha_required;
    char *captcha_id;
    char *captcha_image; /* base64 (картинку рисует приложение) */
    char *captcha_hint;
    char *server_config_json;  /* на успехе — распакованный конфиг ($WIREGUARD_CLIENT_PRIVATE_KEY цел) */
    char *raw_response_json;   /* сырое расшифрованное тело (service_info/supported_protocols) */
} agw_import_result;

/* Результат v1/subscriptions (App Store). vpn_key — без префикса "vpn://"; crc — qChecksum/CRC-16. */
typedef struct
{
    int error;
    char *server_config_json;
    char *vpn_key;
    unsigned short crc;
} agw_app_store_result;

AGW_API agw_json_result agw_get_services(agw_client *client, const char *os_version, const char *app_version,
                                         const char *cli_name, const char *app_language);

AGW_API agw_json_result agw_get_news(agw_client *client, const char *locale, const char *const *country_codes,
                                     size_t country_count, const char *const *service_types, size_t service_count);

AGW_API agw_url_result agw_get_updater_endpoint(agw_client *client, const char *cli_version, const char *os_version,
                                                const char *installation_uuid);

AGW_API agw_account_info_result agw_get_account_info(agw_client *client, const agw_gateway_request *req,
                                                     const char *cli_version, const char *subscription_status);

AGW_API agw_url_result agw_get_renewal_link(agw_client *client, const agw_gateway_request *req,
                                            const char *cli_version, const char *subscription_status);

AGW_API agw_import_result agw_import_service(agw_client *client, const agw_gateway_request *req,
                                             const char *public_key);

AGW_API agw_import_result agw_import_trial(agw_client *client, const agw_gateway_request *req, const char *public_key,
                                           const char *email);

AGW_API agw_import_result agw_resolve_import_captcha(agw_client *client, const agw_gateway_request *req,
                                                     const char *public_key, const char *captcha_id,
                                                     const char *captcha_solution);

AGW_API int agw_deactivate_device(agw_client *client, const agw_gateway_request *req);

AGW_API agw_app_store_result agw_import_from_app_store(agw_client *client, const agw_gateway_request *req,
                                                       const char *public_key, const char *transaction_id);

AGW_API void agw_json_result_free(agw_json_result *result);
AGW_API void agw_url_result_free(agw_url_result *result);
AGW_API void agw_account_info_result_free(agw_account_info_result *result);
AGW_API void agw_import_result_free(agw_import_result *result);
AGW_API void agw_app_store_result_free(agw_app_store_result *result);

#ifdef __cplusplus
}
#endif

#endif
