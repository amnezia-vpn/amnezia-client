#ifndef AGW_FAILOVER_PROXY_LIST_H
#define AGW_FAILOVER_PROXY_LIST_H

#include <string>
#include <vector>

#include "agw/types.h"

namespace agw::failover
{
    std::vector<std::string> buildStorageUrls(const std::vector<std::string> &primaryBaseUrls,
                                              const std::vector<std::string> &fallbackBaseUrls,
                                              const FailoverContext &ctx);

    std::vector<std::string> decodeProxyList(const std::string &body, bool isDevEnvironment,
                                             const std::string &pubKeyPem);
}

#endif
