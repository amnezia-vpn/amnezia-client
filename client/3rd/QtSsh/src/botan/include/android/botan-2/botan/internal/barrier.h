/*
* Barrier
* (C) 2016 Joel Low
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_UTIL_BARRIER_H_
#define BOTAN_UTIL_BARRIER_H_

#include <mutex>
#include <condition_variable>

namespace Botan {

/**
Barrier implements a barrier synchronization primitive. wait() will
indicate how many threads to synchronize; each thread needing
synchronization should call sync(). When sync() returns, the barrier
is reset to zero, and the m_syncs counter is incremented. m_syncs is a
counter to ensure that wait() can be called after a sync() even if the
previously sleeping threads have not awoken.)
*/
class Barrier final
   {
   public:
      explicit Barrier(int value = 0) : m_value(value), m_syncs(0) {}

      void wait(size_t delta);

      void sync();

   private:
      size_t m_value;
      size_t m_syncs;
      std::mutex m_mutex;
      std::condition_variable m_cond;
   };

}

#endif
