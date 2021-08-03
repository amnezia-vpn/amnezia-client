/*
* Diffie-Hellman
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DIFFIE_HELLMAN_H_
#define BOTAN_DIFFIE_HELLMAN_H_

#include <botan/dl_algo.h>

namespace Botan {

/**
* This class represents Diffie-Hellman public keys.
*/
class BOTAN_PUBLIC_API(2,0) DH_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const override { return "DH"; }

      std::vector<uint8_t> public_value() const;

      DL_Group::Format group_format() const override { return DL_Group::ANSI_X9_42; }

      /**
      * Create a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      DH_PublicKey(const AlgorithmIdentifier& alg_id,
                   const std::vector<uint8_t>& key_bits) :
         DL_Scheme_PublicKey(alg_id, key_bits, DL_Group::ANSI_X9_42) {}

      /**
      * Construct a public key with the specified parameters.
      * @param grp the DL group to use in the key
      * @param y the public value y
      */
      DH_PublicKey(const DL_Group& grp, const BigInt& y);
   protected:
      DH_PublicKey() = default;
   };

/**
* This class represents Diffie-Hellman private keys.
*/
class BOTAN_PUBLIC_API(2,0) DH_PrivateKey final : public DH_PublicKey,
                                public PK_Key_Agreement_Key,
                                public virtual DL_Scheme_PrivateKey
   {
   public:
      std::vector<uint8_t> public_value() const override;

      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits PKCS #8 structure
      */
      DH_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const secure_vector<uint8_t>& key_bits);

      /**
      * Create a private key.
      * @param rng random number generator to use
      * @param grp the group to be used in the key
      * @param x the key's secret value (or if zero, generate a new key)
      */
      DH_PrivateKey(RandomNumberGenerator& rng, const DL_Group& grp,
                    const BigInt& x = 0);

      std::unique_ptr<PK_Ops::Key_Agreement>
         create_key_agreement_op(RandomNumberGenerator& rng,
                                 const std::string& params,
                                 const std::string& provider) const override;
   };

}

#endif
