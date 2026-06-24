#ifndef AGW_HTTP_CURL_CLIENT_H
#define AGW_HTTP_CURL_CLIENT_H

#include "agw/http.h"

namespace agw {

// Реализация IHttpClient на libcurl (blocking). Компилируется только при наличии libcurl
// (AGW_HAVE_CURL). Per-handle, потокобезопасна при использовании отдельных инстансов на поток.
class CurlHttpClient : public IHttpClient {
public:
    CurlHttpClient();
    ~CurlHttpClient() override;
    HttpResponse send(const HttpRequest &request) override;
};

} // namespace agw

#endif // AGW_HTTP_CURL_CLIENT_H
