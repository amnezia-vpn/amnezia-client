/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_THREAD_POOL_H_
#define BOTAN_THREAD_POOL_H_

#include <botan/types.h>
#include <functional>
#include <deque>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>
#include <mutex>
#include <thread>
#include <future>
#include <condition_variable>

namespace Botan {

class BOTAN_TEST_API Thread_Pool
   {
   public:
      /**
      * Return an instance to a shared thread pool
      */
      static Thread_Pool& global_instance();

      /**
      * Initialize a thread pool with some number of threads
      * @param pool_size number of threads in the pool, if 0
      *        then some default value is chosen
      */
      Thread_Pool(size_t pool_size = 0);

      ~Thread_Pool() { shutdown(); }

      void shutdown();

      size_t worker_count() const { return m_workers.size(); }

      Thread_Pool(const Thread_Pool&) = delete;
      Thread_Pool& operator=(const Thread_Pool&) = delete;

      Thread_Pool(Thread_Pool&&) = delete;
      Thread_Pool& operator=(Thread_Pool&&) = delete;

      /*
      * Enqueue some work
      */
      void queue_thunk(std::function<void ()>);

      template<class F, class... Args>
      auto run(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
         {
         typedef typename std::result_of<F(Args...)>::type return_type;

         auto future_work = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
         auto task = std::make_shared<std::packaged_task<return_type ()>>(future_work);
         auto future_result = task->get_future();
         queue_thunk([task]() { (*task)(); });
         return future_result;
         }

   private:
      void worker_thread();

      // Only touched in constructor and destructor
      std::vector<std::thread> m_workers;

      std::mutex m_mutex;
      std::condition_variable m_more_tasks;
      std::deque<std::function<void ()>> m_tasks;
      bool m_shutdown;
   };

}

#endif
