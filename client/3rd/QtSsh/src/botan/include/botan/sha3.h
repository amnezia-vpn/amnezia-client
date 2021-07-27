/*
* SHA-3
* (C) 2010,2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SHA3_H_
#define BOTAN_SHA3_H_

#include <botan/hash.h>
#include <botan/secmem.h>
#include <string>

BOTAN_FUTURE_INTERNAL_HEADER(sha3.h)

namespace Botan {

/**
* SHA-3
*/
class BOTAN_PUBLIC_API(2,0) SHA_3 : public HashFunction
   {
   public:

      /**
      * @param output_bits the size of the hash output; must be one of
      *                    224, 256, 384, or 512
      */
      explicit SHA_3(size_t output_bits);

      size_t hash_block_size() const override { return m_bitrate / 8; }
      size_t output_length() const override { return m_output_bits / 8; }

      HashFunction* clone() const override;
      std::unique_ptr<HashFunction> copy_state() const override;
      std::string name() const override;
      void clear() override;
      std::string provider() const override;

      // Static functions for internal usage

      /**
      * Absorb data into the provided state
      * @param bitrate the bitrate to absorb into the sponge
      * @param S the sponge state
      * @param S_pos where to begin absorbing into S
      * @param input the input data
      * @param length size of input in bytes
      */
      static size_t absorb(size_t bitrate,
                           secure_vector<uint64_t>& S, size_t S_pos,
                           const uint8_t input[], size_t length);

      /**
      * Add final padding and permute. The padding is assumed to be
      * init_pad || 00... || fini_pad
      *
      * @param bitrate the bitrate to absorb into the sponge
      * @param S the sponge state
      * @param S_pos where to begin absorbing into S
      * @param init_pad the leading pad bits
      * @param fini_pad the final pad bits
      */
      static void finish(size_t bitrate,
                         secure_vector<uint64_t>& S, size_t S_pos,
                         uint8_t init_pad, uint8_t fini_pad);

      /**
      * Expand from provided state
      * @param bitrate sponge parameter
      * @param S the state
      * @param output the output buffer
      * @param output_length the size of output in bytes
      */
      static void expand(size_t bitrate,
                         secure_vector<uint64_t>& S,
                         uint8_t output[], size_t output_length);

      /**
      * The bare Keccak-1600 permutation
      */
      static void permute(uint64_t A[25]);

   private:
      void add_data(const uint8_t input[], size_t length) override;
      void final_result(uint8_t out[]) override;

#if defined(BOTAN_HAS_SHA3_BMI2)
      static void permute_bmi2(uint64_t A[25]);
#endif

      size_t m_output_bits, m_bitrate;
      secure_vector<uint64_t> m_S;
      size_t m_S_pos;
   };

/**
* SHA-3-224
*/
class BOTAN_PUBLIC_API(2,0) SHA_3_224 final : public SHA_3
   {
   public:
      SHA_3_224() : SHA_3(224) {}
   };

/**
* SHA-3-256
*/
class BOTAN_PUBLIC_API(2,0) SHA_3_256 final : public SHA_3
   {
   public:
      SHA_3_256() : SHA_3(256) {}
   };

/**
* SHA-3-384
*/
class BOTAN_PUBLIC_API(2,0) SHA_3_384 final : public SHA_3
   {
   public:
      SHA_3_384() : SHA_3(384) {}
   };

/**
* SHA-3-512
*/
class BOTAN_PUBLIC_API(2,0) SHA_3_512 final : public SHA_3
   {
   public:
      SHA_3_512() : SHA_3(512) {}
   };

}

#endif
