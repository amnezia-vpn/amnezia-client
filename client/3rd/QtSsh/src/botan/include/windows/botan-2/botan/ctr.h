/*
* CTR-BE Mode
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CTR_BE_H_
#define BOTAN_CTR_BE_H_

#include <botan/block_cipher.h>
#include <botan/stream_cipher.h>

BOTAN_FUTURE_INTERNAL_HEADER(ctr.h)

namespace Botan {

/**
* CTR-BE (Counter mode, big-endian)
*/
class BOTAN_PUBLIC_API(2,0) CTR_BE final : public StreamCipher
   {
   public:
      void cipher(const uint8_t in[], uint8_t out[], size_t length) override;

      void set_iv(const uint8_t iv[], size_t iv_len) override;

      size_t default_iv_length() const override;

      bool valid_iv_length(size_t iv_len) const override;

      Key_Length_Specification key_spec() const override;

      std::string name() const override;

      CTR_BE* clone() const override;

      void clear() override;

      /**
      * @param cipher the block cipher to use
      */
      explicit CTR_BE(BlockCipher* cipher);

      CTR_BE(BlockCipher* cipher, size_t ctr_size);

      void seek(uint64_t offset) override;
   private:
      void key_schedule(const uint8_t key[], size_t key_len) override;
      void add_counter(const uint64_t counter);

      std::unique_ptr<BlockCipher> m_cipher;

      const size_t m_block_size;
      const size_t m_ctr_size;
      const size_t m_ctr_blocks;

      secure_vector<uint8_t> m_counter, m_pad;
      std::vector<uint8_t> m_iv;
      size_t m_pad_pos;
   };

}

#endif
