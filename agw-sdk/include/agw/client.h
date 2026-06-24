#ifndef AGW_CLIENT_H
#define AGW_CLIENT_H

#include <memory>
#include <string>

#include "agw/config.h"
#include "agw/types.h"

namespace agw {

// Долгоживущий клиент шлюза. Состояние (кеш прокси) — на инстанс, потокобезопасно (Фаза 4).
class GatewayClient {
public:
    explicit GatewayClient(Config config);
    ~GatewayClient();

    GatewayClient(GatewayClient &&) noexcept;
    GatewayClient &operator=(GatewayClient &&) noexcept;
    GatewayClient(const GatewayClient &) = delete;
    GatewayClient &operator=(const GatewayClient &) = delete;

    // Синхронный POST: блокирует поток вызывающего. endpoint — формат-строка с "%1" под хост.
    // payload — уже сериализованное тело запроса (SDK его не парсит). Возвращает расшифрованное
    // тело и код; body несётся и при ненулевом error.
    Response post(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx);

    // postAsync / postFuture — Фаза 4.

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace agw

#endif // AGW_CLIENT_H
