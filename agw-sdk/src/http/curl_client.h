#ifndef AGW_HTTP_CURL_CLIENT_H
#define AGW_HTTP_CURL_CLIENT_H

#include "agw/http.h"

namespace agw
{
    class CurlHttpClient : public IHttpClient
    {
    public:
        CurlHttpClient();
        ~CurlHttpClient() override;
        HttpResponse send(const HttpRequest &request) override;
    };
}

#endif
