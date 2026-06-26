#ifndef AGW_GATEWAY_CONTROLLER_H
#define AGW_GATEWAY_CONTROLLER_H

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "agw/cancellation.h"
#include "agw/config.h"
#include "agw/types.h"

namespace agw {
class GatewayController {
public:
    explicit GatewayController(Config config);
    ~GatewayController();

    GatewayController(GatewayController &&) noexcept;
    GatewayController &operator=(GatewayController &&) noexcept;
    GatewayController(const GatewayController &) = delete;
    GatewayController &operator=(const GatewayController &) = delete;

    Response post(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx,
                  CancellationToken *cancel = nullptr);

    void postAsync(const std::string &endpoint, const std::string &payload,
                   std::function<void(Response)> onResult, const FailoverContext &ctx,
                   CancellationToken *cancel = nullptr);

    std::future<Response> postFuture(const std::string &endpoint, const std::string &payload,
                                     const FailoverContext &ctx, CancellationToken *cancel = nullptr);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};
}

#endif
