#include "failover/proxy_picker.h"

namespace agw::failover
{

    std::string pickHealthyProxy(IHttpClient &http, const std::vector<std::string> &proxyUrls, int timeoutMsecs)
    {
        for (const auto &proxy : proxyUrls) {
            HttpRequest req;
            req.url = proxy + "lmbd-health";
            req.method = "GET";
            req.headers = { { "Content-Type", "application/json" } };
            req.timeoutMsecs = timeoutMsecs;

            const HttpResponse resp = http.send(req);
            if (resp.error == TransportError::None && !resp.sslError) {
                return proxy;
            }
        }
        return { };
    }

} // namespace agw::failover
