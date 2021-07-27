/*
* PBKDF2
* (C) 1999-2007,2012 Jack Lloyd
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PBKDF2_H_
#define BOTAN_PBKDF2_H_

#include <botan/pbkdf.h>
#include <botan/pwdhash.h>
#include <botan/mac.h>

BOTAN_FUTURE_INTERNAL_HEADER(pbkdf2.h)

namespace Botan {

BOTAN_PUBLIC_API(2,0) size_t pbkdf2(MessageAuthenticationCode& prf,
                        uint8_t out[],
                        size_t out_len,
                        const std::string& passphrase,
                        const uint8_t salt[], size_t salt_len,
                        size_t iterations,
                        std::chrono::milliseconds msec);

/**
* Perform PBKDF2. The prf is assumed to be keyed already.
*/
BOTAN_PUBLIC_API(2,8) void pbkdf2(MessageAuthenticationCode& prf,
                                  uint8_t out[], size_t out_len,
                                  const uint8_t salt[], size_t salt_len,
                                  size_t iterations);

/**
* PBKDF2
*/
class BOTAN_PUBLIC_API(2,8) PBKDF2 final : public PasswordHash
   {
   public:
      PBKDF2(const MessageAuthenticationCode& prf, size_t iter) :
         m_prf(prf.clone()),
         m_iterations(iter)
         {}

      PBKDF2(const MessageAuthenticationCode& prf, size_t olen, std::chrono::milliseconds msec);

      size_t iterations() const override { return m_iterations; }

      std::string to_string() const override;

      void derive_key(uint8_t out[], size_t out_len,
                      const char* password, size_t password_len,
                      const uint8_t salt[], size_t salt_len) const override;
   private:
      std::unique_ptr<MessageAuthenticationCode> m_prf;
      size_t m_iterations;
   };

/**
* Family of PKCS #5 PBKDF2 operations
*/
class BOTAN_PUBLIC_API(2,8) PBKDF2_Family final : public PasswordHashFamily
   {
   public:
      PBKDF2_Family(MessageAuthenticationCode* prf) : m_prf(prf) {}

      std::string name() const override;

      std::unique_ptr<PasswordHash> tune(size_t output_len,
                                         std::chrono::milliseconds msec,
                                         size_t max_memory) const override;

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
      std::unique_ptr<MessageAuthenticationCode> m_prf;
   };

/**
* PKCS #5 PBKDF2 (old interface)
*/
class BOTAN_PUBLIC_API(2,0) PKCS5_PBKDF2 final : public PBKDF
   {
   public:
      std::string name() const override;

      PBKDF* clone() const override;

      size_t pbkdf(uint8_t output_buf[], size_t output_len,
                   const std::string& passphrase,
                   const uint8_t salt[], size_t salt_len,
                   size_t iterations,
                   std::chrono::milliseconds msec) const override;

      /**
      * Create a PKCS #5 instance using the specified message auth code
      * @param mac_fn the MAC object to use as PRF
      */
      explicit PKCS5_PBKDF2(MessageAuthenticationCode* mac_fn) : m_mac(mac_fn) {}
   private:
      std::unique_ptr<MessageAuthenticationCode> m_mac;
   };

}

#endif
