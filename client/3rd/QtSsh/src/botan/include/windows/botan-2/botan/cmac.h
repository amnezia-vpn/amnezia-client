/*
* CMAC
* (C) 1999-2007,2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CMAC_H_
#define BOTAN_CMAC_H_

#include <botan/mac.h>
#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cmac.h)

namespace Botan {

/**
* CMAC, also known as OMAC1
*/
class BOTAN_PUBLIC_API(2,0) CMAC final : public MessageAuthenticationCode
   {
   public:
      std::string name() const override;
      size_t output_length() const override { return m_block_size; }
      MessageAuthenticationCode* clone() const override;

      void clear() override;

      Key_Length_Specification key_spec() const override
         {
         return m_cipher->key_spec();
         }

      /**
      * CMAC's polynomial doubling operation
      *
      * This function was only exposed for use elsewhere in the library, but it is not
      * longer used. This function will be removed in a future release.
      *
      * @param in the input
      */
      static secure_vector<uint8_t>
         BOTAN_DEPRECATED("This was only for internal use and is no longer used")
         poly_double(const secure_vector<uint8_t>& in);

      /**
      * @param cipher the block cipher to use
      */
      explicit CMAC(BlockCipher* cipher);

      CMAC(const CMAC&) = delete;
      CMAC& operator=(const CMAC&) = delete;
   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      void key_schedule(const uint8_t[], size_t) override;

      std::unique_ptr<BlockCipher> m_cipher;
      secure_vector<uint8_t> m_buffer, m_state, m_B, m_P;
      const size_t m_block_size;
      size_t m_position;
   };

}

#endif
