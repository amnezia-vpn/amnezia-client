/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RWLOCK_H_
#define BOTAN_RWLOCK_H_

#include <botan/types.h>
#include <mutex>
#include <condition_variable>

namespace Botan {

/**
* A read-write lock. Writers are favored.
*/
class BOTAN_TEST_API RWLock final
   {
   public:
      RWLock();

      void lock();
      void unlock();

      void lock_shared();
      void unlock_shared();
   private:
      std::mutex m_mutex;
      std::condition_variable m_gate1;
      std::condition_variable m_gate2;
      uint32_t m_state;

      // 2**31 concurrent readers should be enough for anyone
      static const uint32_t is_writing = static_cast<uint32_t>(1) << 31;
      static const uint32_t readers_mask = ~is_writing;
   };

}

#endif
