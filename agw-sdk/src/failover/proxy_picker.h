#ifndef AGW_FAILOVER_PROXY_PICKER_H
#define AGW_FAILOVER_PROXY_PICKER_H

#include <string>
#include <vector>

#include "agw/http.h"

namespace agw::failover {

// Health-check: для каждого прокси по порядку делает GET <proxy>lmbd-health (с таймаутом);
// возвращает первый, ответивший без ошибки, иначе "". Паритет с health-веткой bypassProxy.
std::string pickHealthyProxy(IHttpClient &http, const std::vector<std::string> &proxyUrls, int timeoutMsecs);

} // namespace agw::failover

#endif // AGW_FAILOVER_PROXY_PICKER_H
