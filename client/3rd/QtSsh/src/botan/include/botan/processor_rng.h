/*
* (C) 2016,2019,2020 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RNG_PROCESSOR_RNG_H_
#define BOTAN_RNG_PROCESSOR_RNG_H_

#include <botan/rng.h>

namespace Botan {

/**
* Directly invokes a CPU specific instruction to generate random numbers.
* On x86, the RDRAND instruction is used.
* on POWER, the DARN instruction is used.
*/
class BOTAN_PUBLIC_API(2,15) Processor_RNG final : public Hardware_RNG
   {
   public:
      /**
      * Constructor will throw if CPU does not have RDRAND bit set
      */
      Processor_RNG();

      /**
      * Return true if RNG instruction is available on the current processor
      */
      static bool available();

      bool accepts_input() const override { return false; }
      bool is_seeded() const override { return true; }

      void randomize(uint8_t out[], size_t out_len) override;

      /*
      * No way to provide entropy to RDRAND generator, so add_entropy is ignored
      */
      void add_entropy(const uint8_t[], size_t) override;

      /*
      * No way to reseed processor provided generator, so reseed is ignored
      */
      size_t reseed(Entropy_Sources&, size_t, std::chrono::milliseconds) override;

      std::string name() const override;
   };

}

#endif
