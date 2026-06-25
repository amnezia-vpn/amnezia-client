#ifndef AGW_CLIENT_H
#define AGW_CLIENT_H

#include <functional>
#include <future>
#include <memory>
#include <string>

#include "agw/cancellation.h"
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
    // тело и код; body несётся и при ненулевом error. cancel опционален.
    Response post(const std::string &endpoint, const std::string &payload, const FailoverContext &ctx,
                  CancellationToken *cancel = nullptr);

    // Асинхронный POST: задача уходит в пул, onResult вызывается НА ПОТОКЕ ПУЛА (маршалинг в свой
    // поток — забота хоста). cancel опционален; должен жить до срабатывания коллбэка.
    void postAsync(const std::string &endpoint, const std::string &payload,
                   std::function<void(Response)> onResult, const FailoverContext &ctx,
                   CancellationToken *cancel = nullptr);

    // Тот же пул, результат через future. cancel опционален; должен жить до готовности future.
    std::future<Response> postFuture(const std::string &endpoint, const std::string &payload,
                                     const FailoverContext &ctx, CancellationToken *cancel = nullptr);

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace agw

#endif // AGW_CLIENT_H
