/*
* PKCS#11 Mechanism
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_MECHANISM_H_
#define BOTAN_P11_MECHANISM_H_

#include <botan/p11.h>

#include <utility>
#include <string>
#include <memory>

namespace Botan {
namespace PKCS11 {

/**
* Simple class to build and hold the data for a CK_MECHANISM struct
* for RSA (encryption/decryption, signature/verification)
* and EC (ECDSA signature/verification, ECDH key derivation).
*/
class MechanismWrapper final
   {
   public:
      /// @param mechanism_type the CK_MECHANISM_TYPE for the `mechanism` field of the CK_MECHANISM struct
      explicit MechanismWrapper(MechanismType mechanism_type);

      /**
      * Creates the CK_MECHANISM data for RSA encryption/decryption
      * @param padding supported paddings are Raw (X.509), EME-PKCS1-v1_5 (PKCS#1 v1.5) and OAEP (PKCS#1 OAEP)
      */
      static MechanismWrapper create_rsa_crypt_mechanism(const std::string& padding);

      /**
      * Creates the CK_MECHANISM data for RSA signature/verification
      * @param padding supported paddings are Raw (X.509), EMSA3 (PKCS#1 v1.5), EMSA4 (PKCS#1 PSS),
      * EMSA2 (ANSI X9.31) and ISO9796 (ISO/IEC 9796)
      */
      static MechanismWrapper create_rsa_sign_mechanism(const std::string& padding);

      /**
      * Creates the CK_MECHANISM data for ECDSA signature/verification
      * @param hash the hash algorithm used to hash the data to sign.
      * supported hash functions are Raw and SHA-160 to SHA-512
      */
      static MechanismWrapper create_ecdsa_mechanism(const std::string& hash);

      /**
      * Creates the CK_MECHANISM data for ECDH key derivation (CKM_ECDH1_DERIVE or CKM_ECDH1_COFACTOR_DERIVE)
      * @param params specifies the key derivation function to use.
      * Supported KDFs are Raw and SHA-160 to SHA-512.
      * Params can also include the string "Cofactor" if the cofactor
      * key derivation mechanism should be used, for example "SHA-512,Cofactor"
      */
      static MechanismWrapper create_ecdh_mechanism(const std::string& params);

      /**
      * Sets the salt for the ECDH mechanism parameters.
      * @param salt the salt
      * @param salt_len size of the salt in bytes
      */
      inline void set_ecdh_salt(const uint8_t salt[], size_t salt_len)
         {
         m_parameters->ecdh_params.pSharedData = const_cast<uint8_t*>(salt);
         m_parameters->ecdh_params.ulSharedDataLen = static_cast<Ulong>(salt_len);
         }

      /**
      * Sets the public key of the other party for the ECDH mechanism parameters.
      * @param other_key key of the other party
      * @param other_key_len size of the key of the other party in bytes
      */
      inline void set_ecdh_other_key(const uint8_t other_key[], size_t other_key_len)
         {
         m_parameters->ecdh_params.pPublicData = const_cast<uint8_t*>(other_key);
         m_parameters->ecdh_params.ulPublicDataLen = static_cast<Ulong>(other_key_len);
         }

      /// @return a pointer to the CK_MECHANISM struct that can be passed to the cryptoki functions
      inline Mechanism* data() const
         {
         return const_cast<Mechanism*>(&m_mechanism);
         }

      /// @return the size of the padding in bytes (for encryption/decryption)
      inline size_t padding_size() const
         {
         return m_padding_size;
         }

      /// Holds the mechanism parameters for OAEP, PSS and ECDH
      union MechanismParameters
         {
         MechanismParameters()
            {
            clear_mem(this, 1);
            }

         RsaPkcsOaepParams oaep_params;
         RsaPkcsPssParams pss_params;
         Ecdh1DeriveParams ecdh_params;
         };

   private:
      Mechanism m_mechanism;
      std::shared_ptr<MechanismParameters> m_parameters;
      size_t m_padding_size = 0;
   };

}

}

#endif
