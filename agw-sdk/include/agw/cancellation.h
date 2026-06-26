#ifndef AGW_CANCELLATION_H
#define AGW_CANCELLATION_H

#include <atomic>

namespace agw
{
    class CancellationToken
    {
    public:
        CancellationToken() = default;

        CancellationToken(const CancellationToken &) = delete;
        CancellationToken &operator=(const CancellationToken &) = delete;

        void cancel() noexcept
        {
            m_cancelled.store(true, std::memory_order_relaxed);
        }
        bool isCancelled() const noexcept
        {
            return m_cancelled.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<bool> m_cancelled { false };
    };
}

#endif
