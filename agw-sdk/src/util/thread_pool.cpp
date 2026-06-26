#include "util/thread_pool.h"

namespace agw::util
{
    ThreadPool::ThreadPool(std::size_t threadCount)
    {
        if (threadCount == 0) {
            threadCount = 1;
        }
        m_workers.reserve(threadCount);
        for (std::size_t i = 0; i < threadCount; ++i) {
            m_workers.emplace_back([this] { workerLoop(); });
        }
    }

    ThreadPool::~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_cv.notify_all();
        for (auto &w : m_workers) {
            if (w.joinable()) {
                w.join();
            }
        }
    }

    void ThreadPool::submit(std::function<void()> task)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

    void ThreadPool::workerLoop()
    {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this] { return m_stopping || !m_tasks.empty(); });

                if (m_tasks.empty()) {
                    if (m_stopping) {
                        return;
                    }
                    continue;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
            }
            task();
        }
    }
}
