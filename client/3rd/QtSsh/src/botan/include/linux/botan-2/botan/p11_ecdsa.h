/*
* PKCS#11 ECDSA
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_ECDSA_H_
#define BOTAN_P11_ECDSA_H_

#include <botan/p11.h>
#include <botan/pk_keys.h>

#if defined(BOTAN_HAS_ECDSA)

#include <botan/p11_ecc_key.h>
#include <botan/ecdsa.h>

#include <string>

namespace Botan {
namespace PKCS11 {
class Session;

/// Represents a PKCS#11 ECDSA public key
class BOTAN_PUBLIC_API(2,0) PKCS11_ECDSA_PublicKey final : public PKCS11_EC_PublicKey, public virtual ECDSA_PublicKey
   {
   public:
      /**
      * Creates a PKCS11_ECDSA_PublicKey object from an existing PKCS#11 ECDSA public key
      * @param session the session to use
      * @param handle the handle of the ECDSA public key
      */
      PKCS11_ECDSA_PublicKey(Session& session, ObjectHandle handle)
         : EC_PublicKey(), PKCS11_EC_PublicKey(session, handle)
         {}

      /**
      * Imports an ECDSA public key
      * @param session the session to use
      * @param props the attributes of the public key
      */
      PKCS11_ECDSA_PublicKey(Session& session, const EC_PublicKeyImportProperties& props)
         : EC_PublicKey(), PKCS11_EC_PublicKey(session, props)
         {}

      inline std::string algo_name() const override
         {
         return "ECDSA";
         }

      /// @return the exported ECDSA public key
      ECDSA_PublicKey export_key() const;

      std::unique_ptr<PK_Ops::Verification>
         create_verification_op(const std::string& params,
                                const std::string& provider) const override;
   };

/// Represents a PKCS#11 ECDSA private key
class BOTAN_PUBLIC_API(2,0) PKCS11_ECDSA_PrivateKey final : public PKCS11_EC_PrivateKey
   {
   public:
      /**
      * Creates a PKCS11_ECDSA_PrivateKey object from an existing PKCS#11 ECDSA private key
      * @param session the session to use
      * @param handle the handle of the ECDSA private key
      */
      PKCS11_ECDSA_PrivateKey(Session& session, ObjectHandle handle)
         : PKCS11_EC_PrivateKey(session, handle)
         {}

      /**
      * Imports a ECDSA private key
      * @param session the session to use
      * @param props the attributes of the private key
      */
      PKCS11_ECDSA_PrivateKey(Session& session, const EC_PrivateKeyImportProperties& props)
         : PKCS11_EC_PrivateKey(session, props)
         {}

      /**
      * Generates a PKCS#11 ECDSA private key
      * @param session the session to use
      * @param ec_params DER-encoding of an ANSI X9.62 Parameters value
      * @param props the attributes of the private key
      * @note no persistent public key object will be created
      */
      PKCS11_ECDSA_PrivateKey(Session& session, const std::vector<uint8_t>& ec_params,
                              const EC_PrivateKeyGenerationProperties& props)
         : PKCS11_EC_PrivateKey(session, ec_params, props)
         {}

      inline std::string algo_name() const override
         {
         return "ECDSA";
         }

      size_t message_parts() const override { return 2; }

      size_t message_part_size() const override
         { return domain().get_order().bytes(); }

      /// @return the exported ECDSA private key
      ECDSA_PrivateKey export_key() const;

      secure_vector<uint8_t> private_key_bits() const override;

      bool check_key(RandomNumberGenerator&, bool) const override;

      std::unique_ptr<PK_Ops::Signature>
         create_signature_op(RandomNumberGenerator& rng,
                             const std::string& params,
                             const std::string& provider) const override;
   };

using PKCS11_ECDSA_KeyPair = std::pair<PKCS11_ECDSA_PublicKey, PKCS11_ECDSA_PrivateKey>;

/**
* ECDSA key pair generation
* @param session the session that should be used for the key generation
* @param pub_props the properties of the public key
* @param priv_props the properties of the private key
*/
BOTAN_PUBLIC_API(2,0) PKCS11_ECDSA_KeyPair generate_ecdsa_keypair(Session& session,
      const EC_PublicKeyGenerationProperties& pub_props, const EC_PrivateKeyGenerationProperties& priv_props);
}

}

#endif
#endif
