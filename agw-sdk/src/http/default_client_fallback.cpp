#ifndef AGW_HAVE_CURL

    #include <stdexcept>

    #include "agw/http.h"

namespace agw
{

    // Сборка без libcurl: транспорта по умолчанию нет, клиент обязан передать свой IHttpClient
    // через Config. makeDefaultHttpClient в таком случае бросает.
    std::unique_ptr<IHttpClient> makeDefaultHttpClient()
    {
        throw std::runtime_error("agw: SDK built without libcurl; provide Config::httpClient (your own IHttpClient)");
    }

} // namespace agw

#endif // !AGW_HAVE_CURL
