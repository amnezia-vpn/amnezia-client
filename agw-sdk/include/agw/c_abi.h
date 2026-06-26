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

#ifdef __cplusplus
}
#endif

#endif
