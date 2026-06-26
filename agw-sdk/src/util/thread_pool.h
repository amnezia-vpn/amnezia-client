#ifndef AGW_UTIL_THREAD_POOL_H
#define AGW_UTIL_THREAD_POOL_H

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace agw::util
{
    class ThreadPool
    {
    public:
        explicit ThreadPool(std::size_t threadCount);
        ~ThreadPool();

        ThreadPool(const ThreadPool &) = delete;
        ThreadPool &operator=(const ThreadPool &) = delete;

        void submit(std::function<void()> task);

    private:
        void workerLoop();

        std::vector<std::thread> m_workers;
        std::queue<std::function<void()>> m_tasks;
        std::mutex m_mutex;
        std::condition_variable m_cv;
        bool m_stopping = false;
    };
}

#endif
