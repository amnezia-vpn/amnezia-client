/*
* GOST 34.10-2001
* (C) 2007 Falko Strenzke, FlexSecure GmbH
*          Manuel Hartl, FlexSecure GmbH
* (C) 2008-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_GOST_3410_KEY_H_
#define BOTAN_GOST_3410_KEY_H_

#include <botan/ecc_key.h>

namespace Botan {

/**
* GOST-34.10 Public Key
*/
class BOTAN_PUBLIC_API(2,0) GOST_3410_PublicKey : public virtual EC_PublicKey
   {
   public:

      /**
      * Construct a public key from a given public point.
      * @param dom_par the domain parameters associated with this key
      * @param public_point the public point defining this key
      */
      GOST_3410_PublicKey(const EC_Group& dom_par,
                          const PointGFp& public_point) :
         EC_PublicKey(dom_par, public_point) {}

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      GOST_3410_PublicKey(const AlgorithmIdentifier& alg_id,
                          const std::vector<uint8_t>& key_bits);

      /**
      * Get this keys algorithm name.
      * @result this keys algorithm name
      */
      std::string algo_name() const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override
         { return domain().get_order().bytes(); }

      Signature_Format default_x509_signature_format() const override
         { return IEEE_1363; }

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;

   protected:
      GOST_3410_PublicKey() = default;
   };

/**
* GOST-34.10 Private Key
*/
class BOTAN_PUBLIC_API(2,0) GOST_3410_PrivateKey final :
   public GOST_3410_PublicKey, public EC_PrivateKey
   {
   public:
      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits ECPrivateKey bits
      */
      GOST_3410_PrivateKey(const AlgorithmIdentifier& alg_id,
                           const secure_vector<uint8_t>& key_bits) :
         EC_PrivateKey(alg_id, key_bits) {}

      /**
      * Generate a new private key
      * @param rng a random number generator
      * @param domain parameters to used for this key
      * @param x the private key; if zero, a new random key is generated
      */
      GOST_3410_PrivateKey(RandomNumberGenerator& rng,
                           const EC_Group& domain,
                           const BigInt& x = 0);

      AlgorithmIdentifier pkcs8_algorithm_identifier() const override
         { return EC_PublicKey::algorithm_identifier(); }

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;
   };

}

#endif
