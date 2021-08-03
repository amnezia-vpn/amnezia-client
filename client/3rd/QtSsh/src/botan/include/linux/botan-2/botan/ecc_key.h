/*
* ECDSA
* (C) 2007 Falko Strenzke, FlexSecure GmbH
*          Manuel Hartl, FlexSecure GmbH
* (C) 2008-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ECC_PUBLIC_KEY_BASE_H_
#define BOTAN_ECC_PUBLIC_KEY_BASE_H_

#include <botan/ec_group.h>
#include <botan/pk_keys.h>

namespace Botan {

/**
* This class represents abstract ECC public keys. When encoding a key
* via an encoder that can be accessed via the corresponding member
* functions, the key will decide upon its internally stored encoding
* information whether to encode itself with or without domain
* parameters, or using the domain parameter oid. Furthermore, a public
* key without domain parameters can be decoded. In that case, it
* cannot be used for verification until its domain parameters are set
* by calling the corresponding member function.
*/
class BOTAN_PUBLIC_API(2,0) EC_PublicKey : public virtual Public_Key
   {
   public:
      /**
      * Create a public key.
      * @param dom_par EC domain parameters
      * @param pub_point public point on the curve
      */
      EC_PublicKey(const EC_Group& dom_par,
                   const PointGFp& pub_point);

      /**
      * Load a public key.
      * @param alg_id the X.509 algorithm identifier
      * @param key_bits DER encoded public key bits
      */
      EC_PublicKey(const AlgorithmIdentifier& alg_id,
                   const std::vector<uint8_t>& key_bits);

      EC_PublicKey(const EC_PublicKey& other) = default;
      EC_PublicKey& operator=(const EC_PublicKey& other) = default;
      virtual ~EC_PublicKey() = default;

      /**
      * Get the public point of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the public point of this key
      */
      const PointGFp& public_point() const { return m_public_key; }

      AlgorithmIdentifier algorithm_identifier() const override;

      std::vector<uint8_t> public_key_bits() const override;

      bool check_key(RandomNumberGenerator& rng,
                     bool strong) const override;

      /**
      * Get the domain parameters of this key.
      * @throw Invalid_State is thrown if the
      * domain parameters of this point are not set
      * @result the domain parameters of this key
      */
      const EC_Group& domain() const { return m_domain_params; }

      /**
      * Set the domain parameter encoding to be used when encoding this key.
      * @param enc the encoding to use
      */
      void set_parameter_encoding(EC_Group_Encoding enc);

      /**
      * Set the point encoding method to be used when encoding this key.
      * @param enc the encoding to use
      */
      void set_point_encoding(PointGFp::Compression_Type enc);

      /**
      * Return the DER encoding of this keys domain in whatever format
      * is preset for this particular key
      */
      std::vector<uint8_t> DER_domain() const
         { return domain().DER_encode(domain_format()); }

      /**
      * Get the domain parameter encoding to be used when encoding this key.
      * @result the encoding to use
      */
      EC_Group_Encoding domain_format() const
         { return m_domain_encoding; }

      /**
      * Get the point encoding method to be used when encoding this key.
      * @result the encoding to use
      */
      PointGFp::Compression_Type point_encoding() const
         { return m_point_encoding; }

      size_t key_length() const override;
      size_t estimated_strength() const override;

   protected:
      EC_PublicKey() : m_domain_params{}, m_public_key{}, m_domain_encoding(EC_DOMPAR_ENC_EXPLICIT)
      {}

      EC_Group m_domain_params;
      PointGFp m_public_key;
      EC_Group_Encoding m_domain_encoding;
      PointGFp::Compression_Type m_point_encoding = PointGFp::UNCOMPRESSED;
   };

/**
* This abstract class represents ECC private keys
*/
class BOTAN_PUBLIC_API(2,0) EC_PrivateKey : public virtual EC_PublicKey,
                                public virtual Private_Key
   {
   public:
      /*
      * If x=0, creates a new private key in the domain
      * using the given rng. If with_modular_inverse is set,
      * the public key will be calculated by multiplying
      * the base point with the modular inverse of
      * x (as in ECGDSA and ECKCDSA), otherwise by
      * multiplying directly with x (as in ECDSA).
      */
      EC_PrivateKey(RandomNumberGenerator& rng,
                    const EC_Group& domain,
                    const BigInt& x,
                    bool with_modular_inverse=false);

      /*
      * Creates a new private key object from the
      * ECPrivateKey structure given in key_bits.
      * If with_modular_inverse is set,
      * the public key will be calculated by multiplying
      * the base point with the modular inverse of
      * x (as in ECGDSA and ECKCDSA), otherwise by
      * multiplying directly with x (as in ECDSA).
      */
      EC_PrivateKey(const AlgorithmIdentifier& alg_id,
                    const secure_vector<uint8_t>& key_bits,
                    bool with_modular_inverse=false);

      secure_vector<uint8_t> private_key_bits() const override;

      /**
      * Get the private key value of this key object.
      * @result the private key value of this key object
      */
      const BigInt& private_value() const;

      EC_PrivateKey(const EC_PrivateKey& other) = default;
      EC_PrivateKey& operator=(const EC_PrivateKey& other) = default;
      ~EC_PrivateKey() = default;
   protected:
      EC_PrivateKey() = default;

      BigInt m_private_key;
   };

}

#endif
