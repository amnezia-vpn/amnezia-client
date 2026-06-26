#include "agw_test.h"

#include <atomic>
#include <memory>

#include "util/thread_pool.h"

using namespace agw;

int main()
{
    {
        std::atomic<int> counter{0};
        {
            util::ThreadPool pool(4);
            for (int i = 0; i < 1000; ++i) {
                pool.submit([&counter] { counter.fetch_add(1, std::memory_order_relaxed); });
            }
        }
        CHECK(counter.load() == 1000);
    }

    {
        std::atomic<bool> ran{false};
        {
            util::ThreadPool pool(0);
            pool.submit([&ran] { ran.store(true); });
        }
        CHECK(ran.load());
    }

    return AGW_TEST_MAIN_RETURN();
}
