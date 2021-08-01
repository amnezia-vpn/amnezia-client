/*
* TLS CBC+HMAC AEAD
* (C) 2016 Jack Lloyd
* (C) 2016 Daniel Neus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_CBC_HMAC_AEAD_H_
#define BOTAN_TLS_CBC_HMAC_AEAD_H_

#include <botan/aead.h>
#include <botan/block_cipher.h>
#include <botan/mac.h>
#include <botan/tls_version.h>

namespace Botan {

namespace TLS {

/**
* TLS CBC+HMAC AEAD base class (GenericBlockCipher in TLS spec)
* This is the weird TLS-specific mode, not for general consumption.
*/
class BOTAN_TEST_API TLS_CBC_HMAC_AEAD_Mode : public AEAD_Mode
   {
   public:
      size_t process(uint8_t buf[], size_t sz) override final;

      std::string name() const override final;

      void set_associated_data(const uint8_t ad[], size_t ad_len) override;

      size_t update_granularity() const override final;

      Key_Length_Specification key_spec() const override final;

      bool valid_nonce_length(size_t nl) const override final;

      size_t tag_size() const override final { return m_tag_size; }

      size_t default_nonce_length() const override final { return m_iv_size; }

      void clear() override final;

      void reset() override final;

 protected:
      TLS_CBC_HMAC_AEAD_Mode(Cipher_Dir direction,
                             std::unique_ptr<BlockCipher> cipher,
                             std::unique_ptr<MessageAuthenticationCode> mac,
                             size_t cipher_keylen,
                             size_t mac_keylen,
                             Protocol_Version version,
                             bool use_encrypt_then_mac);

      size_t cipher_keylen() const { return m_cipher_keylen; }
      size_t mac_keylen() const { return m_mac_keylen; }
      size_t iv_size() const { return m_iv_size; }
      size_t block_size() const { return m_block_size; }

      bool use_encrypt_then_mac() const { return m_use_encrypt_then_mac; }

      bool is_datagram_protocol() const { return m_is_datagram; }

      Cipher_Mode& cbc() const { return *m_cbc; }

      MessageAuthenticationCode& mac() const
         {
         BOTAN_ASSERT_NONNULL(m_mac);
         return *m_mac;
         }

      secure_vector<uint8_t>& cbc_state() { return m_cbc_state; }
      std::vector<uint8_t>& assoc_data() { return m_ad; }
      secure_vector<uint8_t>& msg() { return m_msg; }

      std::vector<uint8_t> assoc_data_with_len(uint16_t len);

   private:
      void start_msg(const uint8_t nonce[], size_t nonce_len) override final;

      void key_schedule(const uint8_t key[], size_t length) override final;

      const std::string m_cipher_name;
      const std::string m_mac_name;
      size_t m_cipher_keylen;
      size_t m_mac_keylen;
      size_t m_iv_size;
      size_t m_tag_size;
      size_t m_block_size;
      bool m_use_encrypt_then_mac;
      bool m_is_datagram;

      std::unique_ptr<Cipher_Mode> m_cbc;
      std::unique_ptr<MessageAuthenticationCode> m_mac;

      secure_vector<uint8_t> m_cbc_state;
      std::vector<uint8_t> m_ad;
      secure_vector<uint8_t> m_msg;
   };

/**
* TLS_CBC_HMAC_AEAD Encryption
*/
class BOTAN_TEST_API TLS_CBC_HMAC_AEAD_Encryption final : public TLS_CBC_HMAC_AEAD_Mode
   {
   public:
      /**
      */
      TLS_CBC_HMAC_AEAD_Encryption(
                             std::unique_ptr<BlockCipher> cipher,
                             std::unique_ptr<MessageAuthenticationCode> mac,
                             const size_t cipher_keylen,
                             const size_t mac_keylen,
                             const Protocol_Version version,
                             bool use_encrypt_then_mac) :
         TLS_CBC_HMAC_AEAD_Mode(ENCRYPTION,
                                std::move(cipher),
                                std::move(mac),
                                cipher_keylen,
                                mac_keylen,
                                version,
                                use_encrypt_then_mac)
         {}

      void set_associated_data(const uint8_t ad[], size_t ad_len) override;

      size_t output_length(size_t input_length) const override;

      size_t minimum_final_size() const override { return 0; }

      void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) override;
   private:
      void cbc_encrypt_record(secure_vector<uint8_t>& buffer, size_t offset,
                              size_t padding_length);
   };

/**
* TLS_CBC_HMAC_AEAD Decryption
*/
class BOTAN_TEST_API TLS_CBC_HMAC_AEAD_Decryption final : public TLS_CBC_HMAC_AEAD_Mode
   {
   public:
      /**
      */
      TLS_CBC_HMAC_AEAD_Decryption(std::unique_ptr<BlockCipher> cipher,
                                   std::unique_ptr<MessageAuthenticationCode> mac,
                                   const size_t cipher_keylen,
                                   const size_t mac_keylen,
                                   const Protocol_Version version,
                                   bool use_encrypt_then_mac) :
         TLS_CBC_HMAC_AEAD_Mode(DECRYPTION,
                                std::move(cipher),
                                std::move(mac),
                                cipher_keylen,
                                mac_keylen,
                                version,
                                use_encrypt_then_mac)
         {}

      size_t output_length(size_t input_length) const override;

      size_t minimum_final_size() const override { return tag_size(); }

      void finish(secure_vector<uint8_t>& final_block, size_t offset = 0) override;

   private:
      void cbc_decrypt_record(uint8_t record_contents[], size_t record_len);

      void perform_additional_compressions(size_t plen, size_t padlen);
   };

/**
* Check the TLS padding of a record
* @param record the record bits
* @param record_len length of record
* @return 0 if padding is invalid, otherwise padding_bytes + 1
*/
BOTAN_TEST_API uint16_t check_tls_cbc_padding(const uint8_t record[], size_t record_len);

}

}

#endif
