#ifndef AGW_CONFIG_H
#define AGW_CONFIG_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "agw/http.h"
#include "agw/types.h"

namespace agw
{
    struct Config
    {
        std::string gatewayEndpoint;

        std::string agwPublicKeyPem;

        std::vector<std::string> s3PrimaryEndpoints;
        std::vector<std::string> s3FallbackEndpoints;

        bool isDevEnvironment = false;

        int requestTimeoutMsecs = 12000;
        int proxyHealthTimeoutMsecs = 1000;
        int proxyStorageTimeoutMsecs = 3000;
        int threadPoolSize = 4;

        std::function<void(const std::string &host)> onBeforeRequest;

        std::function<void(LogLevel, const std::string &message)> log;

        std::shared_ptr<IHttpClient> httpClient;
    };
}

#endif
