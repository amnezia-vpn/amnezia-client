/*
* Ed25519
* (C) 2017 Ribose Inc
*
* Based on the public domain code from SUPERCOP ref10 by
* Peter Schwabe, Daniel J. Bernstein, Niels Duif, Tanja Lange, Bo-Yin Yang
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ED25519_H_
#define BOTAN_ED25519_H_

#include <botan/pk_keys.h>

namespace Botan {

class BOTAN_PUBLIC_API(2,2) Ed25519_PublicKey : public virtual Public_Key
   {
   public:
      std::string algo_name() const override { return "Ed25519"; }

      size_t estimated_strength() const override { return 128; }

      size_t key_length() const override { return 255; }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      /**
      * Create a Ed25519 Public Key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      Ed25519_PublicKey(const AlgorithmIdentifier& alg_id,
                        const std::vector<uint8_t>& key_bits);

      template<typename Alloc>
      Ed25519_PublicKey(const std::vector<uint8_t, Alloc>& pub) :
         Ed25519_PublicKey(pub.data(), pub.size()) {}

      Ed25519_PublicKey(const uint8_t pub_key[], size_t len);

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;

      const std::vector<uint8_t>& get_public_key() const { return m_public; }

   protected:
      Ed25519_PublicKey() = default;
      std::vector<uint8_t> m_public;
   };

class BOTAN_PUBLIC_API(2,2) Ed25519_PrivateKey final : public Ed25519_PublicKey,
                                     public virtual Private_Key
   {
   public:
      /**
      * Construct a private key from the specified parameters.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      Ed25519_PrivateKey(const AlgorithmIdentifier& alg_id,
                         const secure_vector<uint8_t>& key_bits);

      /**
      * Generate a private key.
      * @param rng the RNG to use
      */
      explicit Ed25519_PrivateKey(RandomNumberGenerator& rng);

      /**
      * Construct a private key from the specified parameters.
      * @param secret_key the private key
      */
      explicit Ed25519_PrivateKey(const secure_vector<uint8_t>& secret_key);

      const secure_vector<uint8_t>& get_private_key() const { return m_private; }

      secure_vector<uint8_t> private_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;

   private:
      secure_vector<uint8_t> m_private;
   };

void ed25519_gen_keypair(uint8_t pk[32], uint8_t sk[64], const uint8_t seed[32]);

void ed25519_sign(uint8_t sig[64],
                  const uint8_t msg[],
                  size_t msg_len,
                  const uint8_t sk[64],
                  const uint8_t domain_sep[], size_t domain_sep_len);

bool ed25519_verify(const uint8_t msg[],
                    size_t msg_len,
                    const uint8_t sig[64],
                    const uint8_t pk[32],
                    const uint8_t domain_sep[], size_t domain_sep_len);

}

#endif
