/*
* CBC-MAC
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CBC_MAC_H_
#define BOTAN_CBC_MAC_H_

#include <botan/mac.h>
#include <botan/block_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(cbc_mac.h)

namespace Botan {

/**
* CBC-MAC
*/
class BOTAN_PUBLIC_API(2,0) CBC_MAC final : public MessageAuthenticationCode
   {
   public:
      std::string name() const override;
      MessageAuthenticationCode* clone() const override;
      size_t output_length() const override { return m_cipher->block_size(); }
      void clear() override;

      Key_Length_Specification key_spec() const override
         {
         return m_cipher->key_spec();
         }

      /**
      * @param cipher the block cipher to use
      */
      explicit CBC_MAC(BlockCipher* cipher);
   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      void key_schedule(const uint8_t[], size_t) override;

      std::unique_ptr<BlockCipher> m_cipher;
      secure_vector<uint8_t> m_state;
      size_t m_position = 0;
   };

}

#endif
