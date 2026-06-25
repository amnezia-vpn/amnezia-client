#ifndef AGW_C_ABI_H
#define AGW_C_ABI_H

/*
 * Тонкая C-ABI обёртка над C++ API. Первый потребитель — Dart/Flutter (dart:ffi умеет только C).
 * На границе — только C-типы. Все строки — NUL-terminated UTF-8. Возвращаемое тело (agw_response.body)
 * владеется вызывающим: освобождать через agw_response_free (в т.ч. в async-коллбэке).
 */

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

typedef struct {
    const char *gateway_endpoint;       /* формат-строка с "%1" под хост */
    const char *agw_public_key_pem;     /* PEM публичного RSA-ключа шлюза */

    const char *const *s3_primary_endpoints;  /* массив C-строк (может быть NULL при count==0) */
    size_t s3_primary_count;
    const char *const *s3_fallback_endpoints;
    size_t s3_fallback_count;

    int is_dev_environment;             /* 0/1 */
    int request_timeout_msecs;          /* <=0 → дефолт SDK */
    int proxy_health_timeout_msecs;     /* <=0 → дефолт */
    int proxy_storage_timeout_msecs;    /* <=0 → дефолт */
    int thread_pool_size;               /* <=0 → дефолт */

    agw_before_request_fn on_before_request;  /* nullable */
    void *on_before_request_user_data;
    agw_log_fn log;                            /* nullable */
    void *log_user_data;
} agw_config;

typedef struct {
    int error;        /* числовое значение agw::ErrorCode (0 = NoError) */
    char *body;       /* heap, NUL-terminated; освобождать agw_response_free. NULL при ошибке аллокации */
    size_t body_len;  /* длина тела без учёта NUL (тело может быть бинарным) */
} agw_response;

/* Коллбэк async-POST. response владеется коллбэком: освободить agw_response_free. */
typedef void (*agw_post_callback)(agw_response response, void *user_data);

/* Жизненный цикл клиента. create возвращает NULL при ошибке. */
AGW_API agw_client *agw_client_create(const agw_config *config);
AGW_API void agw_client_destroy(agw_client *client);

/* Синхронный POST (блокирует). cancel_token nullable. Тело результата освобождать agw_response_free. */
AGW_API agw_response agw_client_post(agw_client *client, const char *endpoint, const char *payload,
                                     const char *service_type, const char *user_country_code,
                                     agw_cancel_token *cancel_token);

/* Асинхронный POST: callback вызывается на потоке пула SDK. cancel_token и user_data nullable. */
AGW_API void agw_client_post_async(agw_client *client, const char *endpoint, const char *payload,
                                   const char *service_type, const char *user_country_code,
                                   agw_post_callback callback, void *user_data,
                                   agw_cancel_token *cancel_token);

AGW_API void agw_response_free(agw_response *response);

/* Токен отмены (best-effort). cancel можно звать из другого потока. Должен жить до завершения операции. */
AGW_API agw_cancel_token *agw_cancel_token_create(void);
AGW_API void agw_cancel_token_cancel(agw_cancel_token *token);
AGW_API void agw_cancel_token_destroy(agw_cancel_token *token);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AGW_C_ABI_H */
