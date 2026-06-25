#ifdef AGW_HAVE_CURL

#include "http/curl_client.h"

#include <mutex>
#include <string>

#include <curl/curl.h>

namespace agw {

namespace {

std::once_flag g_curlInitOnce;

void ensureCurlGlobalInit()
{
    std::call_once(g_curlInitOnce, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
}

std::size_t writeCallback(char *ptr, std::size_t size, std::size_t nmemb, void *userdata)
{
    const std::size_t total = size * nmemb;
    auto *buf = static_cast<std::string *>(userdata);
    buf->append(ptr, total);
    return total;
}

// Прерывание трансфера по запросу отмены: ненулевой возврат → CURLE_ABORTED_BY_CALLBACK.
int xferCallback(void *clientp, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
{
    auto *check = static_cast<const std::function<bool()> *>(clientp);
    if (check && *check && (*check)()) {
        return 1;
    }
    return 0;
}

TransportError mapCurlError(CURLcode code, bool &sslError)
{
    sslError = false;
    switch (code) {
    case CURLE_OK:
        return TransportError::None;
    case CURLE_OPERATION_TIMEDOUT:
        return TransportError::Timeout;
    case CURLE_ABORTED_BY_CALLBACK:
        return TransportError::Canceled;
    case CURLE_SSL_CONNECT_ERROR:
    case CURLE_PEER_FAILED_VERIFICATION:
    case CURLE_SSL_CERTPROBLEM:
    case CURLE_SSL_CIPHER:
    case CURLE_SSL_CACERT_BADFILE:
    case CURLE_SSL_ISSUER_ERROR:
        sslError = true;
        return TransportError::ConnectionError;
    default:
        return TransportError::ConnectionError;
    }
}

} // namespace

CurlHttpClient::CurlHttpClient()
{
    ensureCurlGlobalInit();
}

CurlHttpClient::~CurlHttpClient() = default;

HttpResponse CurlHttpClient::send(const HttpRequest &request)
{
    HttpResponse response;

    CURL *curl = curl_easy_init();
    if (!curl) {
        response.error = TransportError::ConnectionError;
        response.errorString = "curl_easy_init failed";
        return response;
    }

    struct curl_slist *headers = nullptr;
    for (const auto &h : request.headers) {
        const std::string line = h.first + ": " + h.second;
        headers = curl_slist_append(headers, line.c_str());
    }

    curl_easy_setopt(curl, CURLOPT_URL, request.url.c_str());
    if (headers) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }
    if (request.timeoutMsecs > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(request.timeoutMsecs));
    }
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    // Паритет с QNetworkAccessManager: тот автоматически распаковывает gzip/deflate-ответы.
    // "" = объявляем все поддерживаемые curl кодировки и прозрачно их декодируем (иначе тело
    // придёт сжатым и AES-расшифровка ответа шлюза упадёт → ApiConfigDecryptionError).
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");

    if (request.cancelCheck) {
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferCallback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &request.cancelCheck);
    }

    if (request.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.body.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(request.body.size()));
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    const CURLcode code = curl_easy_perform(curl);
    bool sslError = false;
    response.error = mapCurlError(code, sslError);
    response.sslError = sslError;
    if (code != CURLE_OK) {
        response.errorString = curl_easy_strerror(code);
    }

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    response.httpStatusCode = static_cast<int>(httpCode);

    if (headers) {
        curl_slist_free_all(headers);
    }
    curl_easy_cleanup(curl);
    return response;
}

std::unique_ptr<IHttpClient> makeDefaultHttpClient()
{
    return std::make_unique<CurlHttpClient>();
}

} // namespace agw

#endif // AGW_HAVE_CURL
