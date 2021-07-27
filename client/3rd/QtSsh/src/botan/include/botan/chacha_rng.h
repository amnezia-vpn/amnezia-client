/*
* ChaCha_RNG
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CHACHA_RNG_H_
#define BOTAN_CHACHA_RNG_H_

#include <botan/stateful_rng.h>
#include <botan/stream_cipher.h>
#include <botan/mac.h>

namespace Botan {

class Entropy_Sources;

/**
* ChaCha_RNG is a very fast but completely ad-hoc RNG created by
* creating a 256-bit random value and using it as a key for ChaCha20.
*
* The RNG maintains two 256-bit keys, one for HMAC_SHA256 (HK) and the
* other for ChaCha20 (CK). To compute a new key in response to
* reseeding request or add_entropy calls, ChaCha_RNG computes
*   CK' = HMAC_SHA256(HK, input_material)
* Then a new HK' is computed by running ChaCha20 with the new key to
* output 32 bytes:
*   HK' = ChaCha20(CK')
*
* Now output can be produced by continuing to produce output with ChaCha20
* under CK'
*
* The first HK (before seeding occurs) is taken as the all zero value.
*
* @warning This RNG construction is probably fine but is non-standard.
* The primary reason to use it is in cases where the other RNGs are
* not fast enough.
*/
class BOTAN_PUBLIC_API(2,3) ChaCha_RNG final : public Stateful_RNG
   {
   public:
      /**
      * Automatic reseeding is disabled completely, as it has no access to
      * any source for seed material.
      *
      * If a fork is detected, the RNG will be unable to reseed itself
      * in response. In this case, an exception will be thrown rather
      * than generating duplicated output.
      */
      ChaCha_RNG();

      /**
      * Provide an initial seed to the RNG, without providing an
      * underlying RNG or entropy source. Automatic reseeding is
      * disabled completely, as it has no access to any source for
      * seed material.
      *
      * If a fork is detected, the RNG will be unable to reseed itself
      * in response. In this case, an exception will be thrown rather
      * than generating duplicated output.
      *
      * @param seed the seed material, should be at least 256 bits
      */
      ChaCha_RNG(const secure_vector<uint8_t>& seed);

      /**
      * Automatic reseeding from @p underlying_rng will take place after
      * @p reseed_interval many requests or after a fork was detected.
      *
      * @param underlying_rng is a reference to some RNG which will be used
      * to perform the periodic reseeding
      * @param reseed_interval specifies a limit of how many times
      * the RNG will be called before automatic reseeding is performed
      */
      ChaCha_RNG(RandomNumberGenerator& underlying_rng,
                 size_t reseed_interval = BOTAN_RNG_DEFAULT_RESEED_INTERVAL);

      /**
      * Automatic reseeding from @p entropy_sources will take place after
      * @p reseed_interval many requests or after a fork was detected.
      *
      * @param entropy_sources will be polled to perform reseeding periodically
      * @param reseed_interval specifies a limit of how many times
      * the RNG will be called before automatic reseeding is performed.
      */
      ChaCha_RNG(Entropy_Sources& entropy_sources,
                 size_t reseed_interval = BOTAN_RNG_DEFAULT_RESEED_INTERVAL);

      /**
      * Automatic reseeding from @p underlying_rng and @p entropy_sources
      * will take place after @p reseed_interval many requests or after
      * a fork was detected.
      *
      * @param underlying_rng is a reference to some RNG which will be used
      * to perform the periodic reseeding
      * @param entropy_sources will be polled to perform reseeding periodically
      * @param reseed_interval specifies a limit of how many times
      * the RNG will be called before automatic reseeding is performed.
      */
      ChaCha_RNG(RandomNumberGenerator& underlying_rng,
                 Entropy_Sources& entropy_sources,
                 size_t reseed_interval = BOTAN_RNG_DEFAULT_RESEED_INTERVAL);

      std::string name() const override { return "ChaCha_RNG"; }

      size_t security_level() const override;

      size_t max_number_of_bytes_per_request() const override { return 0; }

   private:
      void update(const uint8_t input[], size_t input_len) override;

      void generate_output(uint8_t output[], size_t output_len,
                           const uint8_t input[], size_t input_len) override;

      void clear_state() override;

      std::unique_ptr<MessageAuthenticationCode> m_hmac;
      std::unique_ptr<StreamCipher> m_chacha;
   };

}

#endif
