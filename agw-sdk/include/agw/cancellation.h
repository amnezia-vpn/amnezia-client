#ifndef AGW_CANCELLATION_H
#define AGW_CANCELLATION_H

#include <atomic>

namespace agw {

// Кооперативная отмена (best-effort). Каллер владеет токеном и обязан держать его живым до
// завершения операции (срабатывания коллбэка/future). cancel() можно звать из другого потока.
// SDK проверяет флаг между шагами failover и прерывает текущий трансфер (progress-коллбэк curl);
// результат отменённой операции — ErrorCode::Cancelled.
class CancellationToken {
public:
    CancellationToken() = default;

    CancellationToken(const CancellationToken &) = delete;
    CancellationToken &operator=(const CancellationToken &) = delete;

    void cancel() noexcept { m_cancelled.store(true, std::memory_order_relaxed); }
    bool isCancelled() const noexcept { return m_cancelled.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> m_cancelled{false};
};

} // namespace agw

#endif // AGW_CANCELLATION_H
