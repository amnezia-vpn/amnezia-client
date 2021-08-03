/*
* Curve25519
* (C) 2014 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CURVE_25519_H_
#define BOTAN_CURVE_25519_H_

#include <botan/pk_keys.h>

namespace Botan {

class BOTAN_PUBLIC_API(2,0) Curve25519_PublicKey : public virtual Public_Key
   {
   public:
      std::string algo_name() const override { return "Curve25519"; }

      size_t estimated_strength() const override { return 128; }

      size_t key_length() const override { return 255; }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      std::vector<uint8_t> public_value() const { return m_public; }

      /**
      * Create a Curve25519 Public Key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      Curve25519_PublicKey(const AlgorithmIdentifier& alg_id,
                           const std::vector<uint8_t>& key_bits);

      /**
      * Create a Curve25519 Public Key.
      * @param pub 32-byte raw public key
      */
      explicit Curve25519_PublicKey(const std::vector<uint8_t>& pub) : m_public(pub) {}

      /**
      * Create a Curve25519 Public Key.
      * @param pub 32-byte raw public key
      */
      explicit Curve25519_PublicKey(const secure_vector<uint8_t>& pub) :
         m_public(pub.begin(), pub.end()) {}

   protected:
      Curve25519_PublicKey() = default;
      std::vector<uint8_t> m_public;
   };

class BOTAN_PUBLIC_API(2,0) Curve25519_PrivateKey final : public Curve25519_PublicKey,
                                        public virtual Private_Key,
                                        public virtual PK_Key_Agreement_Key
   {
   public:
      /**
      * Construct a private key from the specified parameters.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      Curve25519_PrivateKey(const AlgorithmIdentifier& alg_id,
                            const secure_vector<uint8_t>& key_bits);

      /**
      * Generate a private key.
      * @param rng the RNG to use
      */
      explicit Curve25519_PrivateKey(RandomNumberGenerator& rng);

      /**
      * Construct a private key from the specified parameters.
      * @param secret_key the private key
      */
      explicit Curve25519_PrivateKey(const secure_vector<uint8_t>& secret_key);

      std::vector<uint8_t> public_value() const override { return Curve25519_PublicKey::public_value(); }

      secure_vector<uint8_t> agree(const uint8_t w[], size_t w_len) const;

      const secure_vector<uint8_t>& get_x() const { return m_private; }

      secure_vector<uint8_t> private_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng, bool strong) const override;

      std::unique_ptr<PK_Ops::Key_Agreement>
         create_key_agreement_op(RandomNumberGenerator& rng,
                                 const std::string& params,
                                 const std::string& provider) const override;

   private:
      secure_vector<uint8_t> m_private;
   };

typedef Curve25519_PublicKey X25519_PublicKey;
typedef Curve25519_PrivateKey X25519_PrivateKey;

/*
* The types above are just wrappers for curve25519_donna, plus defining
* encodings for public and private keys.
*/
void BOTAN_PUBLIC_API(2,0) curve25519_donna(uint8_t mypublic[32],
                                const uint8_t secret[32],
                                const uint8_t basepoint[32]);

/**
* Exponentiate by the x25519 base point
* @param mypublic output value
* @param secret random scalar
*/
void BOTAN_PUBLIC_API(2,0) curve25519_basepoint(uint8_t mypublic[32],
                                    const uint8_t secret[32]);

}

#endif
