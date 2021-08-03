/*
* DES
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DES_H_
#define BOTAN_DES_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(des.h)

namespace Botan {

/**
* DES
*/
class BOTAN_PUBLIC_API(2,0) DES final : public Block_Cipher_Fixed_Params<8, 8>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "DES"; }
      BlockCipher* clone() const override { return new DES; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint32_t> m_round_key;
   };

/**
* Triple DES
*/
class BOTAN_PUBLIC_API(2,0) TripleDES final : public Block_Cipher_Fixed_Params<8, 16, 24, 8>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "TripleDES"; }
      BlockCipher* clone() const override { return new TripleDES; }
   private:
      void key_schedule(const uint8_t[], size_t) override;

      secure_vector<uint32_t> m_round_key;
   };

/*
* DES Tables
*/
extern const uint32_t DES_SPBOX1[256];
extern const uint32_t DES_SPBOX2[256];
extern const uint32_t DES_SPBOX3[256];
extern const uint32_t DES_SPBOX4[256];
extern const uint32_t DES_SPBOX5[256];
extern const uint32_t DES_SPBOX6[256];
extern const uint32_t DES_SPBOX7[256];
extern const uint32_t DES_SPBOX8[256];

}

#endif
