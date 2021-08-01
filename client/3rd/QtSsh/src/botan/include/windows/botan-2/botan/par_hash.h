/*
* Parallel Hash
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PARALLEL_HASH_H_
#define BOTAN_PARALLEL_HASH_H_

#include <botan/hash.h>
#include <vector>

BOTAN_FUTURE_INTERNAL_HEADER(par_hash.h)

namespace Botan {

/**
* Parallel Hashes
*/
class BOTAN_PUBLIC_API(2,0) Parallel final : public HashFunction
   {
   public:
      void clear() override;
      std::string name() const override;
      HashFunction* clone() const override;
      std::unique_ptr<HashFunction> copy_state() const override;

      size_t output_length() const override;

      /**
      * @param hashes a set of hashes to compute in parallel
      * Takes ownership of all pointers
      */
      explicit Parallel(std::vector<std::unique_ptr<HashFunction>>& hashes);

      Parallel(const Parallel&) = delete;
      Parallel& operator=(const Parallel&) = delete;
   private:
      Parallel() = delete;

      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;

      std::vector<std::unique_ptr<HashFunction>> m_hashes;
   };

}

#endif
