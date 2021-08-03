/*
* GOST 28147-89
* (C) 1999-2009 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_GOST_28147_89_H_
#define BOTAN_GOST_28147_89_H_

#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(gost_28147.h)

namespace Botan {

/**
* The GOST 28147-89 block cipher uses a set of 4 bit Sboxes, however
* the standard does not actually define these Sboxes; they are
* considered a local configuration issue. Several different sets are
* used.
*/
class BOTAN_PUBLIC_API(2,0) GOST_28147_89_Params final
   {
   public:
      /**
      * @param row the row
      * @param col the column
      * @return sbox entry at this row/column
      */
      uint8_t sbox_entry(size_t row, size_t col) const;

      /**
      * @return name of this parameter set
      */
      std::string param_name() const { return m_name; }

      /**
      * Return a representation used for building larger tables
      * For internal use
      */
      uint8_t sbox_pair(size_t row, size_t col) const;

      /**
      * Default GOST parameters are the ones given in GOST R 34.11 for
      * testing purposes; these sboxes are also used by Crypto++, and,
      * at least according to Wikipedia, the Central Bank of Russian
      * Federation
      * @param name of the parameter set
      */
      explicit GOST_28147_89_Params(const std::string& name = "R3411_94_TestParam");
   private:
      const uint8_t* m_sboxes;
      std::string m_name;
   };

/**
* GOST 28147-89
*/
class BOTAN_PUBLIC_API(2,0) GOST_28147_89 final : public Block_Cipher_Fixed_Params<8, 32>
   {
   public:
      void encrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;
      void decrypt_n(const uint8_t in[], uint8_t out[], size_t blocks) const override;

      void clear() override;

      std::string name() const override;
      BlockCipher* clone() const override { return new GOST_28147_89(m_SBOX); }

      /**
      * @param params the sbox parameters to use
      */
      explicit GOST_28147_89(const GOST_28147_89_Params& params);

      explicit GOST_28147_89(const std::string& param_name) :
         GOST_28147_89(GOST_28147_89_Params(param_name)) {}
   private:
      explicit GOST_28147_89(const std::vector<uint32_t>& other_SBOX) :
         m_SBOX(other_SBOX), m_EK(8) {}

      void key_schedule(const uint8_t[], size_t) override;

      /*
      * The sbox is not secret, this is just a larger expansion of it
      * which we generate at runtime for faster execution
      */
      std::vector<uint32_t> m_SBOX;

      secure_vector<uint32_t> m_EK;
   };

}

#endif
