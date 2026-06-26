#ifndef AGW_HAVE_CURL

    #include <stdexcept>

    #include "agw/http.h"

namespace agw
{
    std::unique_ptr<IHttpClient> makeDefaultHttpClient()
    {
        throw std::runtime_error("agw: SDK built without libcurl; provide Config::httpClient (your own IHttpClient)");
    }
}

#endif
