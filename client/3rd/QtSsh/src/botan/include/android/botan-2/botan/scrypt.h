/**
* (C) 2018 Jack Lloyd
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SCRYPT_H_
#define BOTAN_SCRYPT_H_

#include <botan/pwdhash.h>

//BOTAN_FUTURE_INTERNAL_HEADER(scrypt.h)

namespace Botan {

/**
* Scrypt key derivation function (RFC 7914)
*/
class BOTAN_PUBLIC_API(2,8) Scrypt final : public PasswordHash
   {
   public:
      Scrypt(size_t N, size_t r, size_t p);

      Scrypt(const Scrypt& other) = default;
      Scrypt& operator=(const Scrypt&) = default;

      /**
      * Derive a new key under the current Scrypt parameter set
      */
      void derive_key(uint8_t out[], size_t out_len,
                      const char* password, size_t password_len,
                      const uint8_t salt[], size_t salt_len) const override;

      std::string to_string() const override;

      size_t N() const { return m_N; }
      size_t r() const { return m_r; }
      size_t p() const { return m_p; }

      size_t iterations() const override { return r(); }

      size_t parallelism() const override { return p(); }

      size_t memory_param() const override { return N(); }

      size_t total_memory_usage() const override;

   private:
      size_t m_N, m_r, m_p;
   };

class BOTAN_PUBLIC_API(2,8) Scrypt_Family final : public PasswordHashFamily
   {
   public:
      std::string name() const override;

      std::unique_ptr<PasswordHash> tune(size_t output_length,
                                         std::chrono::milliseconds msec,
                                         size_t max_memory) const override;

      std::unique_ptr<PasswordHash> default_params() const override;

      std::unique_ptr<PasswordHash> from_iterations(size_t iter) const override;

      std::unique_ptr<PasswordHash> from_params(
         size_t N, size_t r, size_t p) const override;
   };

/**
* Scrypt key derivation function (RFC 7914)
*
* @param output the output will be placed here
* @param output_len length of output
* @param password the user password
* @param password_len length of password
* @param salt the salt
* @param salt_len length of salt
* @param N the CPU/Memory cost parameter, must be power of 2
* @param r the block size parameter
* @param p the parallelization parameter
*
* Suitable parameters for most uses would be N = 32768, r = 8, p = 1
*
* Scrypt uses approximately (p + N + 1) * 128 * r bytes of memory
*/
void BOTAN_PUBLIC_API(2,8) scrypt(uint8_t output[], size_t output_len,
                                  const char* password, size_t password_len,
                                  const uint8_t salt[], size_t salt_len,
                                  size_t N, size_t r, size_t p);

/**
* Scrypt key derivation function (RFC 7914)
* Before 2.8 this function was the primary interface for scrypt
*
* @param output the output will be placed here
* @param output_len length of output
* @param password the user password
* @param salt the salt
* @param salt_len length of salt
* @param N the CPU/Memory cost parameter, must be power of 2
* @param r the block size parameter
* @param p the parallelization parameter
*
* Suitable parameters for most uses would be N = 32768, r = 8, p = 1
*
* Scrypt uses approximately (p + N + 1) * 128 * r bytes of memory
*/
inline void scrypt(uint8_t output[], size_t output_len,
                   const std::string& password,
                   const uint8_t salt[], size_t salt_len,
                   size_t N, size_t r, size_t p)
   {
   return scrypt(output, output_len,
                 password.c_str(), password.size(),
                 salt, salt_len,
                 N, r, p);
   }

inline size_t scrypt_memory_usage(size_t N, size_t r, size_t p)
   {
   return 128 * r * (N + p);
   }

}

#endif
