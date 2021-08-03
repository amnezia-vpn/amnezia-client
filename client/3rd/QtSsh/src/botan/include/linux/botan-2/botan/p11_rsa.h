/*
* PKCS#11 RSA
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_RSA_H_
#define BOTAN_P11_RSA_H_

#include <botan/p11_types.h>
#include <botan/p11_object.h>
#include <botan/pk_keys.h>
#include <botan/bigint.h>

#if defined(BOTAN_HAS_RSA)
#include <botan/rsa.h>
#include <utility>

namespace Botan {
namespace PKCS11 {

/// Properties for generating a PKCS#11 RSA public key
class BOTAN_PUBLIC_API(2,0) RSA_PublicKeyGenerationProperties final : public PublicKeyProperties
   {
   public:
      /// @param bits length in bits of modulus n
      explicit RSA_PublicKeyGenerationProperties(Ulong bits);

      /// @param pub_exponent public exponent e
      inline void set_pub_exponent(const BigInt& pub_exponent = BigInt(0x10001))
         {
         add_binary(AttributeType::PublicExponent, BigInt::encode(pub_exponent));
         }

      virtual ~RSA_PublicKeyGenerationProperties() = default;
   };

/// Properties for importing a PKCS#11 RSA public key
class BOTAN_PUBLIC_API(2,0) RSA_PublicKeyImportProperties final : public PublicKeyProperties
   {
   public:
      /// @param modulus modulus n
      /// @param pub_exponent public exponent e
      RSA_PublicKeyImportProperties(const BigInt& modulus, const BigInt& pub_exponent);

      /// @return the modulus
      inline const BigInt& modulus() const
         {
         return m_modulus;
         }

      /// @return the public exponent
      inline const BigInt& pub_exponent() const
         {
         return m_pub_exponent;
         }

      virtual ~RSA_PublicKeyImportProperties() = default;
   private:
      const BigInt m_modulus;
      const BigInt m_pub_exponent;
   };

/// Represents a PKCS#11 RSA public key
class BOTAN_PUBLIC_API(2,0) PKCS11_RSA_PublicKey : public Object, public RSA_PublicKey
   {
   public:
      static const ObjectClass Class = ObjectClass::PublicKey;

      /**
      * Creates a PKCS11_RSA_PublicKey object from an existing PKCS#11 RSA public key
      * @param session the session to use
      * @param handle the handle of the RSA public key
      */
      PKCS11_RSA_PublicKey(Session& session, ObjectHandle handle);

      /**
      * Imports a RSA public key
      * @param session the session to use
      * @param pubkey_props the attributes of the public key
      */
      PKCS11_RSA_PublicKey(Session& session, const RSA_PublicKeyImportProperties& pubkey_props);

      std::unique_ptr<PK_Ops::Encryption>
         create_encryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;
   };

/// Properties for importing a PKCS#11 RSA private key
class BOTAN_PUBLIC_API(2,0) RSA_PrivateKeyImportProperties final : public PrivateKeyProperties
   {
   public:
      /**
      * @param modulus modulus n
      * @param priv_exponent private exponent d
      */
      RSA_PrivateKeyImportProperties(const BigInt& modulus, const BigInt& priv_exponent);

      /// @param pub_exponent public exponent e
      inline void set_pub_exponent(const BigInt& pub_exponent)
         {
         add_binary(AttributeType::PublicExponent, BigInt::encode(pub_exponent));
         }

      /// @param prime1 prime p
      inline void set_prime_1(const BigInt& prime1)
         {
         add_binary(AttributeType::Prime1, BigInt::encode(prime1));
         }

      /// @param prime2 prime q
      inline void set_prime_2(const BigInt& prime2)
         {
         add_binary(AttributeType::Prime2, BigInt::encode(prime2));
         }

      /// @param exp1 private exponent d modulo p-1
      inline void set_exponent_1(const BigInt& exp1)
         {
         add_binary(AttributeType::Exponent1, BigInt::encode(exp1));
         }

      /// @param exp2 private exponent d modulo q-1
      inline void set_exponent_2(const BigInt& exp2)
         {
         add_binary(AttributeType::Exponent2, BigInt::encode(exp2));
         }

      /// @param coeff CRT coefficient q^-1 mod p
      inline void set_coefficient(const BigInt& coeff)
         {
         add_binary(AttributeType::Coefficient, BigInt::encode(coeff));
         }

      /// @return the modulus
      inline const BigInt& modulus() const
         {
         return m_modulus;
         }

      /// @return the private exponent
      inline const BigInt& priv_exponent() const
         {
         return m_priv_exponent;
         }

      virtual ~RSA_PrivateKeyImportProperties() = default;

   private:
      const BigInt m_modulus;
      const BigInt m_priv_exponent;
   };

/// Properties for generating a PKCS#11 RSA private key
class BOTAN_PUBLIC_API(2,0) RSA_PrivateKeyGenerationProperties final : public PrivateKeyProperties
   {
   public:
      RSA_PrivateKeyGenerationProperties()
         : PrivateKeyProperties(KeyType::Rsa)
         {}

      virtual ~RSA_PrivateKeyGenerationProperties() = default;
   };

/// Represents a PKCS#11 RSA private key
class BOTAN_PUBLIC_API(2,0) PKCS11_RSA_PrivateKey final :
   public Object, public Private_Key, public RSA_PublicKey
   {
   public:
      static const ObjectClass Class = ObjectClass::PrivateKey;

      /// Creates a PKCS11_RSA_PrivateKey object from an existing PKCS#11 RSA private key
      PKCS11_RSA_PrivateKey(Session& session, ObjectHandle handle);

      /**
      * Imports a RSA private key
      * @param session the session to use
      * @param priv_key_props the properties of the RSA private key
      */
      PKCS11_RSA_PrivateKey(Session& session, const RSA_PrivateKeyImportProperties& priv_key_props);

      /**
      * Generates a PKCS#11 RSA private key
      * @param session the session to use
      * @param bits length in bits of modulus n
      * @param priv_key_props the properties of the RSA private key
      * @note no persistent public key object will be created
      */
      PKCS11_RSA_PrivateKey(Session& session, uint32_t bits, const RSA_PrivateKeyGenerationProperties& priv_key_props);

      /// @return the exported RSA private key
      RSA_PrivateKey export_key() const;

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<PK_Ops::Decryption>
         create_decryption_op(RandomNumberGenerator& rng,
                              const std::string& params,
                              const std::string& provider) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;
   };

using PKCS11_RSA_KeyPair = std::pair<PKCS11_RSA_PublicKey, PKCS11_RSA_PrivateKey>;

/**
* RSA key pair generation
* @param session the session that should be used for the key generation
* @param pub_props properties of the public key
* @param priv_props properties of the private key
*/
BOTAN_PUBLIC_API(2,0) PKCS11_RSA_KeyPair generate_rsa_keypair(Session& session, const RSA_PublicKeyGenerationProperties& pub_props,
      const RSA_PrivateKeyGenerationProperties& priv_props);
}

}
#endif

#endif
