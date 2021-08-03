/*
* Mlock Allocator
* (C) 2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MLOCK_ALLOCATOR_H_
#define BOTAN_MLOCK_ALLOCATOR_H_

#include <botan/types.h>
#include <vector>
#include <memory>

BOTAN_FUTURE_INTERNAL_HEADER(locking_allocator.h)

namespace Botan {

class Memory_Pool;

class BOTAN_PUBLIC_API(2,0) mlock_allocator final
   {
   public:
      static mlock_allocator& instance();

      void* allocate(size_t num_elems, size_t elem_size);

      bool deallocate(void* p, size_t num_elems, size_t elem_size) noexcept;

      mlock_allocator(const mlock_allocator&) = delete;

      mlock_allocator& operator=(const mlock_allocator&) = delete;

   private:
      mlock_allocator();

      ~mlock_allocator();

      std::unique_ptr<Memory_Pool> m_pool;
      std::vector<void*> m_locked_pages;
   };

}

#endif
