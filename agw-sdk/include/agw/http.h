#ifndef AGW_HTTP_H
#define AGW_HTTP_H

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace agw
{
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
        std::string method;
        std::string body;
        std::vector<std::pair<std::string, std::string>> headers;
        int timeoutMsecs = 0;

        std::function<bool()> cancelCheck;
    };

    struct HttpResponse
    {
        TransportError error = TransportError::None;
        std::string errorString;
        int httpStatusCode = 0;

        bool sslError = false;
        std::string body;
    };

    class IHttpClient
    {
    public:
        virtual ~IHttpClient() = default;
        virtual HttpResponse send(const HttpRequest &request) = 0;
    };

    std::unique_ptr<IHttpClient> makeDefaultHttpClient();
}

#endif
