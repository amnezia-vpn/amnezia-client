/*
* ECKCDSA (ISO/IEC 14888-3:2006/Cor.2:2009)
* (C) 2016 René Korthaus, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECKCDSA_KEY_H_
#define BOTAN_ECKCDSA_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* This class represents ECKCDSA public keys.
*/
class BOTAN_PUBLIC_API(2,0) ECKCDSA_PublicKey : public virtual EC_PublicKey
   {
   public:

      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      ECKCDSA_PublicKey(const EC_Group& dom_par,
                      const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      ECKCDSA_PublicKey(const AlgorithmIdentifier& alg_id,
                      const std::vector<uint8_t>& key_bits) :
         EC_PublicKey(alg_id, key_bits) {}

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name ("ECGDSA")
      */
      std::string algo_name() const override { return "ECKCDSA"; }

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override
         { return domain().get_order().bytes(); }

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;
   protected:
      ECKCDSA_PublicKey() = default;
   };

/**
* This class represents ECKCDSA private keys.
*/
class BOTAN_PUBLIC_API(2,0) ECKCDSA_PrivateKey final : public ECKCDSA_PublicKey,
                                    public EC_PrivateKey
   {
   public:

      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      ECKCDSA_PrivateKey(const AlgorithmIdentifier& alg_id,
                       const secure_vector<uint8_t>& key_bits) :
         EC_PrivateKey(alg_id, key_bits, true) {}

      /**
      * Create a private key.
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key (if zero, generate a new random key)
      */
      ECKCDSA_PrivateKey(RandomNumberGenerator& rng,
                       const EC_Group& domain,
                       const BigInt& x = 0) :
         EC_PrivateKey(rng, domain, x, true) {}

      bool check_key(RandomNumberGenerator& rng, bool) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;
   };

}

#endif
