/*
* OpenPGP PBKDF
* (C) 1999-2007,2017 Jack Lloyd
* (C) 2018 Ribose Inc
*
* Distributed under the terms of the Botan license
*/

#ifndef BOTAN_OPENPGP_S2K_H_
#define BOTAN_OPENPGP_S2K_H_

#include <botan/pbkdf.h>
#include <botan/pwdhash.h>
#include <botan/hash.h>

/*
This header will not be fully internal - the RFC4880 count
encoding functions will remain here. But the definition of
OpenPGP_S2K will be made internal
*/

//BOTAN_FUTURE_INTERNAL_HEADER(pgp_s2k.h)

namespace Botan {

/**
* RFC 4880 encodes the iteration count to a single-byte value
*/
uint8_t BOTAN_PUBLIC_API(2,8) RFC4880_encode_count(size_t iterations);

/**
* Decode the iteration count from RFC 4880 encoding
*/
size_t BOTAN_PUBLIC_API(2,8) RFC4880_decode_count(uint8_t encoded_iter);

/**
* Round an arbitrary iteration count to next largest iteration count
* supported by RFC4880 encoding.
*/
inline size_t RFC4880_round_iterations(size_t iterations)
   {
   return RFC4880_decode_count(RFC4880_encode_count(iterations));
   }

/**
* OpenPGP's S2K
*
* See RFC 4880 sections 3.7.1.1, 3.7.1.2, and 3.7.1.3
* If the salt is empty and iterations == 1, "simple" S2K is used
* If the salt is non-empty and iterations == 1, "salted" S2K is used
* If the salt is non-empty and iterations > 1, "iterated" S2K is used
*
* Due to complexities of the PGP S2K algorithm, time-based derivation
* is not supported. So if iterations == 0 and msec.count() > 0, an
* exception is thrown. In the future this may be supported, in which
* case "iterated" S2K will be used and the number of iterations
* performed is returned.
*
* Note that unlike PBKDF2, OpenPGP S2K's "iterations" are defined as
* the number of bytes hashed.
*/
class BOTAN_PUBLIC_API(2,2) OpenPGP_S2K final : public PBKDF
   {
   public:
      /**
      * @param hash the hash function to use
      */
      explicit OpenPGP_S2K(HashFunction* hash) : m_hash(hash) {}

      std::string name() const override
         {
         return "OpenPGP-S2K(" + m_hash->name() + ")";
         }

      PBKDF* clone() const override
         {
         return new OpenPGP_S2K(m_hash->clone());
         }

      size_t pbkdf(uint8_t output_buf[], size_t output_len,
                   const std::string& passphrase,
                   const uint8_t salt[], size_t salt_len,
                   size_t iterations,
                   std::chrono::milliseconds msec) const override;

      /**
      * RFC 4880 encodes the iteration count to a single-byte value
      */
      static uint8_t encode_count(size_t iterations)
         {
         return RFC4880_encode_count(iterations);
         }

      static size_t decode_count(uint8_t encoded_iter)
         {
         return RFC4880_decode_count(encoded_iter);
         }

   private:
      std::unique_ptr<HashFunction> m_hash;
   };

/**
* OpenPGP's S2K
*
* See RFC 4880 sections 3.7.1.1, 3.7.1.2, and 3.7.1.3
* If the salt is empty and iterations == 1, "simple" S2K is used
* If the salt is non-empty and iterations == 1, "salted" S2K is used
* If the salt is non-empty and iterations > 1, "iterated" S2K is used
*
* Note that unlike PBKDF2, OpenPGP S2K's "iterations" are defined as
* the number of bytes hashed.
*/
class BOTAN_PUBLIC_API(2,8) RFC4880_S2K final : public PasswordHash
   {
   public:
      /**
      * @param hash the hash function to use
      * @param iterations is rounded due to PGP formatting
      */
      RFC4880_S2K(HashFunction* hash, size_t iterations);

      std::string to_string() const override;

      size_t iterations() const override { return m_iterations; }

      void derive_key(uint8_t out[], size_t out_len,
                      const char* password, size_t password_len,
                      const uint8_t salt[], size_t salt_len) const override;

   private:
      std::unique_ptr<HashFunction> m_hash;
      size_t m_iterations;
   };

class BOTAN_PUBLIC_API(2,8) RFC4880_S2K_Family final : public PasswordHashFamily
   {
   public:
      RFC4880_S2K_Family(HashFunction* hash) : m_hash(hash) {}

      std::string name() const override;

      std::unique_ptr<PasswordHash> tune(size_t output_len,
                                         std::chrono::milliseconds msec,
                                         size_t max_mem) const override;

      /**
      * Return some default parameter set for this PBKDF that should be good
      * enough for most users. The value returned may change over time as
      * processing power and attacks improve.
      */
      std::unique_ptr<PasswordHash> default_params() const override;

      std::unique_ptr<PasswordHash> from_iterations(size_t iter) const override;

      std::unique_ptr<PasswordHash> from_params(
         size_t iter, size_t, size_t) const override;
   private:
      std::unique_ptr<HashFunction> m_hash;
   };

}

#endif
