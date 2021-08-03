/*
* Camellia
* (C) 2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CAMELLIA_H_
#define BOTAN_CAMELLIA_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(camellia.h)

namespace Botan {

/**
* Camellia-128
*/
class BOTAN_PUBLIC_API(2,0) Camellia_128 final : public Block_Cipher_Fixed_Params<16, 16>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "Camellia-128"; }
      BlockCipher* clone() const override { return new Camellia_128; }
   private:
      void key_schedule(const uint8_t key[], size_t length) override;

      secure_vector<uint64_t> m_SK;
   };

/**
* Camellia-192
*/
class BOTAN_PUBLIC_API(2,0) Camellia_192 final : public Block_Cipher_Fixed_Params<16, 24>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "Camellia-192"; }
      BlockCipher* clone() const override { return new Camellia_192; }
   private:
      void key_schedule(const uint8_t key[], size_t length) override;

      secure_vector<uint64_t> m_SK;
   };

/**
* Camellia-256
*/
class BOTAN_PUBLIC_API(2,0) Camellia_256 final : public Block_Cipher_Fixed_Params<16, 32>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;
      std::string name() const override { return "Camellia-256"; }
      BlockCipher* clone() const override { return new Camellia_256; }
   private:
      void key_schedule(const uint8_t key[], size_t length) override;

      secure_vector<uint64_t> m_SK;
   };

}

#endif
