/*
* Format Preserving Encryption (FE1 scheme)
* (C) 2009,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FPE_FE1_H_
#define BOTAN_FPE_FE1_H_

#include <botan/sym_algo.h>
#include <botan/bigint.h>

namespace Botan {

class Modular_Reducer;
class MessageAuthenticationCode;

/**
* Format Preserving Encryption using the scheme FE1 from the paper
* "Format-Preserving Encryption" by Bellare, Rogaway, et al
* (https://eprint.iacr.org/2009/251)
*/
class BOTAN_PUBLIC_API(2,5) FPE_FE1 final : public SymmetricAlgorithm
   {
   public:

      /**
      * @param n the modulus. All plaintext and ciphertext values must be
      *        less than this.
      * @param rounds the number of rounds to use. Must be at least 3.
      * @param compat_mode An error in versions before 2.5.0 chose incorrect
      *        values for a and b. Set compat_mode to true to select this version.
      * @param mac_algo the PRF to use as the encryption function
      */
      FPE_FE1(const BigInt& n,
              size_t rounds = 5,
              bool compat_mode = false,
              const std::string& mac_algo = "HMAC(SHA-256)");

      ~FPE_FE1();

      Key_Length_Specification key_spec() const override;

      std::string name() const override;

      void clear() override;

      /**
      * Encrypt X from and onto the group Z_n using key and tweak
      * @param x the plaintext to encrypt <= n
      * @param tweak will modify the ciphertext
      * @param tweak_len length of tweak
      */
      BigInt encrypt(const BigInt& x, const uint8_t tweak[], size_t tweak_len) const;

      /**
      * Decrypt X from and onto the group Z_n using key and tweak
      * @param x the ciphertext to encrypt <= n
      * @param tweak must match the value used to encrypt
      * @param tweak_len length of tweak
      */
      BigInt decrypt(const BigInt& x, const uint8_t tweak[], size_t tweak_len) const;

      BigInt encrypt(const BigInt& x, uint64_t tweak) const;

      BigInt decrypt(const BigInt& x, uint64_t tweak) const;
   private:
      void key_schedule(const uint8_t key[], size_t length) override;

      BigInt F(const BigInt& R, size_t round,
               const secure_vector<uint8_t>& tweak,
               secure_vector<uint8_t>& tmp) const;

      secure_vector<uint8_t> compute_tweak_mac(const uint8_t tweak[], size_t tweak_len) const;

      std::unique_ptr<MessageAuthenticationCode> m_mac;
      std::unique_ptr<Modular_Reducer> mod_a;
      std::vector<uint8_t> m_n_bytes;
      BigInt m_a;
      BigInt m_b;
      size_t m_rounds;
   };

namespace FPE {

/**
* Format Preserving Encryption using the scheme FE1 from the paper
* "Format-Preserving Encryption" by Bellare, Rogaway, et al
* (https://eprint.iacr.org/2009/251)
*
* Encrypt X from and onto the group Z_n using key and tweak
* @param n the modulus
* @param X the plaintext as a BigInt
* @param key a random key
* @param tweak will modify the ciphertext (think of as an IV)
*
* @warning This function is hardcoded to use only 3 rounds which
* may be insecure for some values of n. Prefer FPE_FE1 class
*/
BigInt BOTAN_PUBLIC_API(2,0) fe1_encrypt(const BigInt& n, const BigInt& X,
                                         const SymmetricKey& key,
                                         const std::vector<uint8_t>& tweak);

/**
* Decrypt X from and onto the group Z_n using key and tweak
* @param n the modulus
* @param X the ciphertext as a BigInt
* @param key is the key used for encryption
* @param tweak the same tweak used for encryption
*
* @warning This function is hardcoded to use only 3 rounds which
* may be insecure for some values of n. Prefer FPE_FE1 class
*/
BigInt BOTAN_PUBLIC_API(2,0) fe1_decrypt(const BigInt& n, const BigInt& X,
                                         const SymmetricKey& key,
                                         const std::vector<uint8_t>& tweak);

}

}

#endif
