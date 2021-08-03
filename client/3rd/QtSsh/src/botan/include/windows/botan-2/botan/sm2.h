/*
* SM2
* (C) 2017 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SM2_KEY_H_
#define BOTAN_SM2_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* This class represents SM2 public keys
*/
class BOTAN_PUBLIC_API(2,2) SM2_PublicKey : public virtual EC_PublicKey
   {
   public:

      /**
      * Create a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      SM2_PublicKey(const EC_Group& dom_par,
                    const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      SM2_PublicKey(const AlgorithmIdentifier& alg_id,
                    const std::vector<uint8_t>& key_bits) :
         EC_PublicKey(alg_id, key_bits) {}

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name
      */
      std::string algo_name() const override;

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override
         { return domain().get_order().bytes(); }

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;

      std::unique_ptr<PK_Ops::Encryption>
         create_encryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;

   protected:
      SM2_PublicKey() = default;
   };

/**
* This class represents SM2 private keys
*/
class BOTAN_PUBLIC_API(2,2) SM2_PrivateKey final :
   public SM2_PublicKey, public EC_PrivateKey
   {
   public:

      /**
      * Load a private key
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      SM2_PrivateKey(const AlgorithmIdentifier& alg_id,
                     const secure_vector<uint8_t>& key_bits);

      /**
      * Create a private key.
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key (if zero, generate a new random key)
      */
      SM2_PrivateKey(RandomNumberGenerator& rng,
                     const EC_Group& domain,
                     const BigInt& x = 0);

      bool check_key(RandomNumberGenerator& rng, bool) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;

      std::unique_ptr<PK_Ops::Decryption>
         create_decryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;

      const BigInt& get_da_inv() const { return m_da_inv; }
   private:
      BigInt m_da_inv;
   };

class HashFunction;

std::vector<uint8_t>
BOTAN_PUBLIC_API(2,5) sm2_compute_za(HashFunction& hash,
                                     const std::string& user_id,
                                     const EC_Group& domain,
                                     const PointGFp& pubkey);

// For compat with versions 2.2 - 2.7
typedef SM2_PublicKey SM2_Signature_PublicKey;
typedef SM2_PublicKey SM2_Encryption_PublicKey;

typedef SM2_PrivateKey SM2_Signature_PrivateKey;
typedef SM2_PrivateKey SM2_Encryption_PrivateKey;

}

#endif
