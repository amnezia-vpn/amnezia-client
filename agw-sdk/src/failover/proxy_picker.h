#ifndef AGW_FAILOVER_PROXY_PICKER_H
#define AGW_FAILOVER_PROXY_PICKER_H

#include <string>
#include <vector>

#include "agw/http.h"

namespace agw::failover
{
    std::string pickHealthyProxy(IHttpClient &http, const std::vector<std::string> &proxyUrls, int timeoutMsecs);
}

#endif
