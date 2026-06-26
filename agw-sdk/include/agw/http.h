#ifndef AGW_HTTP_H
#define AGW_HTTP_H

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace agw
{

    // Нейтральная классификация транспортной ошибки (без Qt/curl-специфики).
    // Соответствие оригиналу (QNetworkReply::NetworkError → checkNetworkReplyErrors):
    //   Timeout/Canceled            → ApiConfigTimeoutError
    //   OperationNotImplemented     → ApiUpdateRequestError
    //   ConnectionError (прочее)    → ApiConfigDownloadError
    enum class TransportError {
        None = 0,
        Timeout,
        Canceled,
        OperationNotImplemented,
        ConnectionError,
    };

    struct HttpRequest
    {
        std::string url;
        std::string method; // "POST" | "GET"
        std::string body;   // тело для POST
        std::vector<std::pair<std::string, std::string>> headers;
        int timeoutMsecs = 0;

        // Кооперативная отмена: если задан и вернёт true во время трансфера — реализация прерывает
        // его (результат TransportError::Canceled). Пусто = отмена не поддерживается на этот запрос.
        std::function<bool()> cancelCheck;
    };

    struct HttpResponse
    {
        TransportError error = TransportError::None;
        std::string errorString;
        int httpStatusCode = 0; // фактический HTTP-код; в маппинге ошибок НЕ используется (берётся
                                // http_status из тела), хранится для логов/диагностики
        bool sslError = false;  // были ли ошибки TLS/сертификата (→ ApiConfigSslError)
        std::string body;       // сырые байты ответа (для POST к шлюзу — зашифрованы AES)
    };

    // Абстракция транспорта. Реализация по умолчанию — libcurl (blocking).
    // Интерфейс публичный: его можно подменить (мок в тестах, свой стек) через Config.
    class IHttpClient
    {
    public:
        virtual ~IHttpClient() = default;
        virtual HttpResponse send(const HttpRequest &request) = 0;
    };

    // Транспорт по умолчанию (libcurl). Бросает, если SDK собран без libcurl —
    // тогда клиент обязан получить свой IHttpClient через Config.
    std::unique_ptr<IHttpClient> makeDefaultHttpClient();

} // namespace agw

#endif // AGW_HTTP_H
