/*
 * GMAC
 * (C) 2016 Matthias Gierlings, René Korthaus
 * (C) 2017 Jack Lloyd
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 */

#ifndef BOTAN_GMAC_H_
#define BOTAN_GMAC_H_

#include <botan/mac.h>

BOTAN_FUTURE_INTERNAL_HEADER(gmac.h)

namespace Botan {

class BlockCipher;
class GHASH;

/**
* GMAC
*
* GMAC requires a unique initialization vector be used for each message.
* This must be provided via the MessageAuthenticationCode::start() API
*/
class BOTAN_PUBLIC_API(2,0) GMAC final : public MessageAuthenticationCode
   {
   public:
      void clear() override;
      std::string name() const override;
      size_t output_length() const override;
      MessageAuthenticationCode* clone() const override;

      Key_Length_Specification key_spec() const override;

      /**
      * Creates a new GMAC instance.
      *
      * @param cipher the underlying block cipher to use
      */
      explicit GMAC(BlockCipher* cipher);

      GMAC(const GMAC&) = delete;
      GMAC& operator=(const GMAC&) = delete;

      ~GMAC();

   private:
      void add_data(const uint8_t[], size_t) override;
      void final_result(uint8_t[]) override;
      void start_msg(const uint8_t nonce[], size_t nonce_len) override;
      void key_schedule(const uint8_t key[], size_t size) override;

      static const size_t GCM_BS = 16;
      std::unique_ptr<BlockCipher> m_cipher;
      std::unique_ptr<GHASH> m_ghash;
      secure_vector<uint8_t> m_aad_buf;
      size_t m_aad_buf_pos;
      bool m_initialized;
   };

}
#endif
