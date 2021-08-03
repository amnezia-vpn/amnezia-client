/*
* DLIES
* (C) 1999-2007 Jack Lloyd
* (C) 2016 Daniel Neus, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DLIES_H_
#define BOTAN_DLIES_H_

#include <botan/pubkey.h>
#include <botan/mac.h>
#include <botan/kdf.h>
#include <botan/dh.h>
#include <botan/cipher_mode.h>

namespace Botan {

/**
* DLIES Encryption
*/
class BOTAN_PUBLIC_API(2,0) DLIES_Encryptor final : public PK_Encryptor
   {
   public:
      /**
      * Stream mode: use KDF to provide a stream of bytes to xor with the message
      *
      * @param own_priv_key own (ephemeral) DH private key
      * @param rng the RNG to use
      * @param kdf the KDF that should be used
      * @param mac the MAC function that should be used
      * @param mac_key_len key length of the MAC function. Default = 20 bytes
      *
      * output = (ephemeral) public key + ciphertext + tag
      */
      DLIES_Encryptor(const DH_PrivateKey& own_priv_key,
                      RandomNumberGenerator& rng,
                      KDF* kdf,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      /**
      * Block cipher mode
      *
      * @param own_priv_key own (ephemeral) DH private key
      * @param rng the RNG to use
      * @param kdf the KDF that should be used
      * @param cipher the block cipher that should be used
      * @param cipher_key_len the key length of the block cipher
      * @param mac the MAC function that should be used
      * @param mac_key_len key length of the MAC function. Default = 20 bytes
      *
      * output = (ephemeral) public key + ciphertext + tag
      */
      DLIES_Encryptor(const DH_PrivateKey& own_priv_key,
                      RandomNumberGenerator& rng,
                      KDF* kdf,
                      Cipher_Mode* cipher,
                      size_t cipher_key_len,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      // Set the other parties public key
      inline void set_other_key(const std::vector<uint8_t>& other_pub_key)
         {
         m_other_pub_key = other_pub_key;
         }

      /// Set the initialization vector for the data encryption method
      inline void set_initialization_vector(const InitializationVector& iv)
         {
         m_iv = iv;
         }

   private:
      std::vector<uint8_t> enc(const uint8_t[], size_t,
                            RandomNumberGenerator&) const override;

      size_t maximum_input_size() const override;

      size_t ciphertext_length(size_t ptext_len) const override;

      std::vector<uint8_t> m_other_pub_key;
      std::vector<uint8_t> m_own_pub_key;
      PK_Key_Agreement m_ka;
      std::unique_ptr<KDF> m_kdf;
      std::unique_ptr<Cipher_Mode> m_cipher;
      const size_t m_cipher_key_len;
      std::unique_ptr<MessageAuthenticationCode> m_mac;
      const size_t m_mac_keylen;
      InitializationVector m_iv;
   };

/**
* DLIES Decryption
*/
class BOTAN_PUBLIC_API(2,0) DLIES_Decryptor final : public PK_Decryptor
   {
   public:
      /**
      * Stream mode: use KDF to provide a stream of bytes to xor with the message
      *
      * @param own_priv_key own (ephemeral) DH private key
      * @param rng the RNG to use
      * @param kdf the KDF that should be used
      * @param mac the MAC function that should be used
      * @param mac_key_len key length of the MAC function. Default = 20 bytes
      *
      * input = (ephemeral) public key + ciphertext + tag
      */
      DLIES_Decryptor(const DH_PrivateKey& own_priv_key,
                      RandomNumberGenerator& rng,
                      KDF* kdf,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      /**
      * Block cipher mode
      *
      * @param own_priv_key own (ephemeral) DH private key
      * @param rng the RNG to use
      * @param kdf the KDF that should be used
      * @param cipher the block cipher that should be used
      * @param cipher_key_len the key length of the block cipher
      * @param mac the MAC function that should be used
      * @param mac_key_len key length of the MAC function. Default = 20 bytes
      *
      * input = (ephemeral) public key + ciphertext + tag
      */
      DLIES_Decryptor(const DH_PrivateKey& own_priv_key,
                      RandomNumberGenerator& rng,
                      KDF* kdf,
                      Cipher_Mode* cipher,
                      size_t cipher_key_len,
                      MessageAuthenticationCode* mac,
                      size_t mac_key_len = 20);

      /// Set the initialization vector for the data decryption method
      inline void set_initialization_vector(const InitializationVector& iv)
         {
         m_iv = iv;
         }

   private:
      secure_vector<uint8_t> do_decrypt(uint8_t& valid_mask,
                                     const uint8_t in[], size_t in_len) const override;

      size_t plaintext_length(size_t ctext_len) const override;

      const size_t m_pub_key_size;
      PK_Key_Agreement m_ka;
      std::unique_ptr<KDF> m_kdf;
      std::unique_ptr<Cipher_Mode> m_cipher;
      const size_t m_cipher_key_len;
      std::unique_ptr<MessageAuthenticationCode> m_mac;
      const size_t m_mac_keylen;
      InitializationVector m_iv;
   };

}

#endif
