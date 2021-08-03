/*
* DL Scheme
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DL_ALGO_H_
#define BOTAN_DL_ALGO_H_

#include <botan/dl_group.h>
#include <botan/pk_keys.h>

namespace Botan {

/**
* This class represents discrete logarithm (DL) public keys.
*/
class BOTAN_PUBLIC_API(2,0) DL_Scheme_PublicKey : public virtual Public_Key
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const override;

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      /**
      * Get the DL domain parameters of this key.
      * @return DL domain parameters of this key
      */
      const DL_Group& get_domain() const { return m_group; }

      /**
      * Get the DL domain parameters of this key.
      * @return DL domain parameters of this key
      */
      const DL_Group& get_group() const { return m_group; }

      /**
      * Get the public value y with y = g^x mod p where x is the secret key.
      */
      const BigInt& get_y() const { return m_y; }

      /**
      * Get the prime p of the underlying DL group.
      * @return prime p
      */
      const BigInt& group_p() const { return m_group.get_p(); }

      /**
      * Get the prime q of the underlying DL group.
      * @return prime q
      */
      const BigInt& group_q() const { return m_group.get_q(); }

      /**
      * Get the generator g of the underlying DL group.
      * @return generator g
      */
      const BigInt& group_g() const { return m_group.get_g(); }

      /**
      * Get the underlying groups encoding format.
      * @return encoding format
      */
      virtual DL_Group::Format group_format() const = 0;

      size_t key_length() const override;
      size_t estimated_strength() const override;

      DL_Scheme_PublicKey& operator=(const DL_Scheme_PublicKey& other) = default;

   protected:
      DL_Scheme_PublicKey() = default;

      /**
      * Create a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      * @param group_format the underlying groups encoding format
      */
      DL_Scheme_PublicKey(const AlgorithmIdentifier& alg_id,
                          const std::vector<uint8_t>& key_bits,
                          DL_Group::Format group_format);

      DL_Scheme_PublicKey(const DL_Group& group, const BigInt& y);

      /**
      * The DL public key
      */
      BigInt m_y;

      /**
      * The DL group
      */
      DL_Group m_group;
   };

/**
* This class represents discrete logarithm (DL) private keys.
*/
class BOTAN_PUBLIC_API(2,0) DL_Scheme_PrivateKey : public virtual DL_Scheme_PublicKey,
                                       public virtual Private_Key
   {
   public:
      bool check_key(RandomNumberGenerator& rng, bool) const override;

      /**
      * Get the secret key x.
      * @return secret key
      */
      const BigInt& get_x() const { return m_x; }

      secure_vector<uint8_t> private_key_bits() const override;

      DL_Scheme_PrivateKey& operator=(const DL_Scheme_PrivateKey& other) = default;

   protected:
      /**
      * Create a private key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded private key bits
      * @param group_format the underlying groups encoding format
      */
      DL_Scheme_PrivateKey(const AlgorithmIdentifier& alg_id,
                           const secure_vector<uint8_t>& key_bits,
                           DL_Group::Format group_format);

      DL_Scheme_PrivateKey() = default;

      /**
      * The DL private key
      */
      BigInt m_x;
   };

}

#endif
