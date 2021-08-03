/*
* Blowfish
* (C) 1999-2011 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BLOWFISH_H_
#define BOTAN_BLOWFISH_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(blowfish.h)

namespace Botan {

/**
* Blowfish
*/
class BOTAN_PUBLIC_API(2,0) Blowfish final : public Block_Cipher_Fixed_Params<8, 1, 56>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      /**
      * Modified EKSBlowfish key schedule, used for bcrypt password hashing
      */
      void salted_set_key(const uint8_t key[], size_t key_length,
                          const uint8_t salt[], size_t salt_length,
                          const size_t workfactor, bool salt_first = false);

      BOTAN_DEPRECATED("Use Blowfish::salted_set_key taking salt length")
      void eks_key_schedule(const uint8_t key[], size_t key_length,
                            const uint8_t salt[16], size_t workfactor)
         {
         salted_set_key(key, key_length, salt, 16, workfactor);
         }

      void clear() override;
      std::string name() const override { return "Blowfish"; }
      BlockCipher* clone() const override { return new Blowfish; }
   private:
      void key_schedule(const uint8_t key[], size_t length) override;

      void key_expansion(const uint8_t key[],
                         size_t key_length,
                         const uint8_t salt[],
                         size_t salt_length);

      void generate_sbox(secure_vector<uint32_t>& box,
                         uint32_t& L, uint32_t& R,
                         const uint8_t salt[],
                         size_t salt_length,
                         size_t salt_off) const;

      secure_vector<uint32_t> m_S, m_P;
   };

}

#endif
