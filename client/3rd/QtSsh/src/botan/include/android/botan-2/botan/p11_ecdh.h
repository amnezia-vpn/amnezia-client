/*
* PKCS#11 ECDH
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_ECDH_H_
#define BOTAN_P11_ECDH_H_

#include <botan/p11.h>

#if defined(BOTAN_HAS_ECDH)

#include <botan/p11_ecc_key.h>
#include <botan/ecdh.h>

#include <string>
#include <vector>

namespace Botan {
namespace PKCS11 {
class Session;

/// Represents a PKCS#11 ECDH public key
class BOTAN_PUBLIC_API(2,0) PKCS11_ECDH_PublicKey : public PKCS11_EC_PublicKey
   {
   public:
      /**
      * Create a PKCS11_ECDH_PublicKey object from an existing PKCS#11 ECDH public key
      * @param session the session to use
      * @param handle the handle of the ECDH public key
      */
      PKCS11_ECDH_PublicKey(Session& session, ObjectHandle handle)
         : EC_PublicKey(), PKCS11_EC_PublicKey(session, handle)
         {}

      /**
      * Imports a ECDH public key
      * @param session the session to use
      * @param props the attributes of the public key
      */
      PKCS11_ECDH_PublicKey(Session& session, const EC_PublicKeyImportProperties& props)
         : EC_PublicKey(), PKCS11_EC_PublicKey(session, props)
         {}

      inline std::string algo_name() const override
         {
         return "ECDH";
         }

      /// @return the exported ECDH public key
      ECDH_PublicKey export_key() const;
   };

/// Represents a PKCS#11 ECDH private key
class BOTAN_PUBLIC_API(2,0) PKCS11_ECDH_PrivateKey final : public virtual PKCS11_EC_PrivateKey, public virtual PK_Key_Agreement_Key
   {
   public:
      /**
      * Creates a PKCS11_ECDH_PrivateKey object from an existing PKCS#11 ECDH private key
      * @param session the session to use
      * @param handle the handle of the ECDH private key
      */
      PKCS11_ECDH_PrivateKey(Session& session, ObjectHandle handle)
         : PKCS11_EC_PrivateKey(session, handle)
         {}

      /**
      * Imports an ECDH private key
      * @param session the session to use
      * @param props the attributes of the private key
      */
      PKCS11_ECDH_PrivateKey(Session& session, const EC_PrivateKeyImportProperties& props)
         : PKCS11_EC_PrivateKey(session, props)
         {}

      /**
      * Generates a PKCS#11 ECDH private key
      * @param session the session to use
      * @param ec_params DER-encoding of an ANSI X9.62 Parameters value
      * @param props the attributes of the private key
      * @note no persistent public key object will be created
      */
      PKCS11_ECDH_PrivateKey(Session& session, const std::vector<uint8_t>& ec_params,
                             const EC_PrivateKeyGenerationProperties& props)
         : PKCS11_EC_PrivateKey(session, ec_params, props)
         {}

      inline std::string algo_name() const override
         {
         return "ECDH";
         }

      inline std::vector<uint8_t> public_value() const override
         {
         return public_point().encode(PointGFp::UNCOMPRESSED);
         }

      /// @return the exported ECDH private key
      ECDH_PrivateKey export_key() const;

      secure_vector<uint8_t> private_key_bits() const override;

      std::unique_ptr<PK_Ops::Key_Agreement>
         create_key_agreement_op(RandomNumberGenerator& rng,
                                 const std::string& params,
                                 const std::string& provider) const override;
   };

using PKCS11_ECDH_KeyPair = std::pair<PKCS11_ECDH_PublicKey, PKCS11_ECDH_PrivateKey>;

/**
* PKCS#11 ECDH key pair generation
* @param session the session that should be used for the key generation
* @param pub_props the properties of the public key
* @param priv_props the properties of the private key
*/
BOTAN_PUBLIC_API(2,0) PKCS11_ECDH_KeyPair generate_ecdh_keypair(Session& session, const EC_PublicKeyGenerationProperties& pub_props,
      const EC_PrivateKeyGenerationProperties& priv_props);
}

}

#endif
#endif
