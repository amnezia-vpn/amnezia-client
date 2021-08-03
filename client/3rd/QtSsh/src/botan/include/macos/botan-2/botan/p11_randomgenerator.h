/*
* PKCS#11 Random Generator
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_RNG_H_
#define BOTAN_P11_RNG_H_

#include <botan/rng.h>
#include <botan/p11_types.h>
#include <botan/entropy_src.h>

#include <string>
#include <functional>

namespace Botan {
namespace PKCS11 {

class Module;

/// A random generator that only fetches random from the PKCS#11 RNG
class BOTAN_PUBLIC_API(2,0) PKCS11_RNG final : public Hardware_RNG
   {
   public:
      /// Initialize the RNG with the PKCS#11 session that provides access to the cryptoki functions
      explicit PKCS11_RNG(Session& session);

      std::string name() const override
         {
         return "PKCS11_RNG";
         }

      /// Always returns true
      bool is_seeded() const override
         {
         return true;
         }

      /// No operation - always returns 0
      size_t reseed(Entropy_Sources&, size_t, std::chrono::milliseconds) override
         {
         return 0;
         }

      /// @return the module used by this RNG
      inline Module& module() const
         {
         return m_session.get().module();
         }

      /// Calls `C_GenerateRandom` to generate random data
      void randomize(uint8_t output[], std::size_t length) override;

      /// Calls `C_SeedRandom` to add entropy to the random generation function of the token/middleware
      void add_entropy(const uint8_t in[], std::size_t length) override;

      // C_SeedRandom may suceed
      bool accepts_input() const override { return true; }

   private:
      const std::reference_wrapper<Session> m_session;
   };
}

}

#endif
