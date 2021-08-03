/*
* Block Cipher Cascade
* (C) 2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CASCADE_H_
#define BOTAN_CASCADE_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cascade.h)

namespace Botan {

/**
* Block Cipher Cascade
*/
class BOTAN_PUBLIC_API(2,0) Cascade_Cipher final : public BlockCipher
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      size_t block_size() const override { return m_block; }

      Key_Length_Specification key_spec() const override
         {
         return Key_Length_Specification(m_cipher1->maximum_keylength() +
                                         m_cipher2->maximum_keylength());
         }

      void clear() override;
      std::string name() const override;
      BlockCipher* clone() const override;

      /**
      * Create a cascade of two block ciphers
      * @param cipher1 the first cipher
      * @param cipher2 the second cipher
      */
      Cascade_Cipher(BlockCipher* cipher1, BlockCipher* cipher2);

      Cascade_Cipher(const Cascade_Cipher&) = delete;
      Cascade_Cipher& operator=(const Cascade_Cipher&) = delete;
   private:
      void key_schedule(const uint8_t[], size_t) override;

      size_t m_block;
      std::unique_ptr<BlockCipher> m_cipher1, m_cipher2;
   };


}

#endif
