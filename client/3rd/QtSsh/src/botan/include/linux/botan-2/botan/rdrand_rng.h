/*
* RDRAND RNG
* (C) 2016,2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RNG_RDRAND_H_
#define BOTAN_RNG_RDRAND_H_

#include <botan/rng.h>

namespace Botan {

class BOTAN_PUBLIC_API(2,0) RDRAND_RNG final : public Hardware_RNG
   {
   public:
      /**
      * Constructor will throw if CPU does not have RDRAND bit set
      */
      BOTAN_DEPRECATED("Use Processor_RNG instead") RDRAND_RNG();

      /**
      * Return true if RDRAND is available on the current processor
      */
      static bool available();

      bool accepts_input() const override { return false; }

      /**
      * Uses RDRAND to produce output
      */
      void randomize(uint8_t out[], size_t out_len) override;

      /*
      * No way to provide entropy to RDRAND generator, so add_entropy is ignored
      */
      void add_entropy(const uint8_t[], size_t) override
         { /* no op */ }

      /*
      * No way to reseed RDRAND generator, so reseed is ignored
      */
      size_t reseed(Entropy_Sources&, size_t, std::chrono::milliseconds) override
         { return 0; /* no op */ }

      std::string name() const override { return "RDRAND"; }

      bool is_seeded() const override { return true; }

      /**
      * On correctly working hardware, RDRAND is always supposed to
      * succeed within a set number of retries. If after that many
      * retries RDRAND has still not suceeded, sets ok = false and
      * returns 0.
      */
      static uint32_t BOTAN_DEPRECATED("Use Processor_RNG::randomize") rdrand_status(bool& ok);

      /*
      * Calls RDRAND until it succeeds, this could hypothetically
      * loop forever on broken hardware.
      */
      static uint32_t BOTAN_DEPRECATED("Use Processor_RNG::randomize") rdrand();
   };

}

#endif
