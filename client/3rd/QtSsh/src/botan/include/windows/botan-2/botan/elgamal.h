/*
* ElGamal
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ELGAMAL_H_
#define BOTAN_ELGAMAL_H_

#include <botan/dl_algo.h>

namespace Botan {

/**
* ElGamal Public Key
*/
class BOTAN_PUBLIC_API(2,0) ElGamal_PublicKey : public virtual DL_Scheme_PublicKey
   {
   public:
      std::string algo_name() const override { return "ElGamal"; }
      DL_Group::Format group_format() const override { return DL_Group::ANSI_X9_42; }

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      ElGamal_PublicKey(const AlgorithmIdentifier& alg_id,
                        const std::vector<uint8_t>& key_bits) :
         DL_Scheme_PublicKey(alg_id, key_bits, DL_Group::ANSI_X9_42)
         {}

      /**
      * Create a public key.
      * @param group the underlying DL group
      * @param y the public value y = g^x mod p
      */
      ElGamal_PublicKey(const DL_Group& group, const BigInt& y);

      std::unique_ptr<PK_Ops::Encryption>
         create_encryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;

   protected:
      ElGamal_PublicKey() = default;
   };

/**
* ElGamal Private Key
*/
class BOTAN_PUBLIC_API(2,0) ElGamal_PrivateKey final : public ElGamal_PublicKey,
                                     public virtual DL_Scheme_PrivateKey
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const override;

      /**
      * Load a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded key bits in ANSI X9.42 format
      */
      ElGamal_PrivateKey(const AlgorithmIdentifier& alg_id,
                         const secure_vector<uint8_t>& key_bits);

      /**
      * Create a private key.
      * @param rng random number generator to use
      * @param group the group to be used in the key
      * @param priv_key the key's secret value (or if zero, generate a new key)
      */
      ElGamal_PrivateKey(RandomNumberGenerator& rng,
                         const DL_Group& group,
                         const BigInt& priv_key = 0);

      std::unique_ptr<PK_Ops::Decryption>
         create_decryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;
   };

}

#endif
