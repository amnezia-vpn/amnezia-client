/*
* (C) 2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_UTIL_MUTEX_H_
#define BOTAN_UTIL_MUTEX_H_

#include <botan/types.h>

#if defined(BOTAN_TARGET_OS_HAS_THREADS)

#include <mutex>

namespace Botan {

template<typename T> using lock_guard_type = std::lock_guard<T>;
typedef std::mutex mutex_type;
typedef std::recursive_mutex recursive_mutex_type;

}

#else

// No threads

namespace Botan {

template<typename Mutex>
class lock_guard final
   {
   public:
      explicit lock_guard(Mutex& m) : m_mutex(m)
         { m_mutex.lock(); }

      ~lock_guard() { m_mutex.unlock(); }

      lock_guard(const lock_guard& other) = delete;
      lock_guard& operator=(const lock_guard& other) = delete;
   private:
      Mutex& m_mutex;
   };

class noop_mutex final
   {
   public:
      void lock() {}
      void unlock() {}
   };

typedef noop_mutex mutex_type;
typedef noop_mutex recursive_mutex_type;
template<typename T> using lock_guard_type = lock_guard<T>;

}

#endif

#endif
