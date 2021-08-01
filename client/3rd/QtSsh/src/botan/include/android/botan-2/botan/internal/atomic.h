/*
 * Atomic
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_ATOMIC_H_
#define BOTAN_ATOMIC_H_

#include <botan/types.h>
#include <atomic>
#include <memory>

//BOTAN_FUTURE_INTERNAL_HEADER(atomic.h)

namespace Botan {

template <typename T>
/**
 * Simple helper class to expand std::atomic with copy constructor and copy
 * assignment operator, i.e. for use as element in a container like
 * std::vector. The construction of instances of this wrapper is NOT atomic
 * and needs to be properly guarded.
 **/
class Atomic final
   {
   public:
      Atomic() = default;
      Atomic(const Atomic& data) : m_data(data.m_data.load()) {}
      Atomic(const std::atomic<T>& data) : m_data(data.load()) {}
      ~Atomic() = default;

      Atomic& operator=(const Atomic& a)
         {
         m_data.store(a.m_data.load());
         return *this;
         }

      Atomic& operator=(const std::atomic<T>& a)
         {
         m_data.store(a.load());
         return *this;
         }

      operator std::atomic<T>& () { return m_data; }
      operator T() { return m_data.load(); }

   private:
      std::atomic<T> m_data;
   };

}

#endif
