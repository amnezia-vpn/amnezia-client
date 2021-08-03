/*
* PKCS#11 X.509
* (C) 2016 Daniel Neus, Sirrix AG
* (C) 2016 Philipp Weber, Sirrix AG
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_P11_X509_H_
#define BOTAN_P11_X509_H_

#include <botan/p11_object.h>

#if defined(BOTAN_HAS_X509_CERTIFICATES)

#include <botan/x509cert.h>
#include <vector>

namespace Botan {
namespace PKCS11 {

class Session;

/// Common attributes of all PKCS#11 X509 certificates
class BOTAN_PUBLIC_API(2,0) X509_CertificateProperties final : public CertificateProperties
   {
   public:
      /**
      * @param subject DER-encoding of the certificate subject name
      * @param value BER-encoding of the certificate
      */
      X509_CertificateProperties(const std::vector<uint8_t>& subject, const std::vector<uint8_t>& value);

      X509_CertificateProperties(const X509_Certificate& cert) :
         X509_CertificateProperties(cert.raw_subject_dn(), cert.BER_encode())
         {}

      /// @param id key identifier for public/private key pair
      inline void set_id(const std::vector<uint8_t>& id)
         {
         add_binary(AttributeType::Id, id);
         }

      /// @param issuer DER-encoding of the certificate issuer name
      inline void set_issuer(const std::vector<uint8_t>& issuer)
         {
         add_binary(AttributeType::Issuer, issuer);
         }

      /// @param serial DER-encoding of the certificate serial number
      inline void set_serial(const std::vector<uint8_t>& serial)
         {
         add_binary(AttributeType::SerialNumber, serial);
         }

      /// @param hash hash value of the subject public key
      inline void set_subject_pubkey_hash(const std::vector<uint8_t>& hash)
         {
         add_binary(AttributeType::HashOfSubjectPublicKey, hash);
         }

      /// @param hash hash value of the issuer public key
      inline void set_issuer_pubkey_hash(const std::vector<uint8_t>& hash)
         {
         add_binary(AttributeType::HashOfIssuerPublicKey, hash);
         }

      /// @param alg defines the mechanism used to calculate `CKA_HASH_OF_SUBJECT_PUBLIC_KEY` and `CKA_HASH_OF_ISSUER_PUBLIC_KEY`
      inline void set_hash_alg(MechanismType alg)
         {
         add_numeric(AttributeType::NameHashAlgorithm, static_cast<Ulong>(alg));
         }

      /// @return the subject
      inline const std::vector<uint8_t>& subject() const
         {
         return m_subject;
         }

      /// @return the BER-encoding of the certificate
      inline const std::vector<uint8_t>& value() const
         {
         return m_value;
         }

   private:
      const std::vector<uint8_t> m_subject;
      const std::vector<uint8_t> m_value;
   };

/// Represents a PKCS#11 X509 certificate
class BOTAN_PUBLIC_API(2,0) PKCS11_X509_Certificate final : public Object, public X509_Certificate
   {
   public:
      static const ObjectClass Class = ObjectClass::Certificate;

      /**
      * Create a PKCS11_X509_Certificate object from an existing PKCS#11 X509 cert
      * @param session the session to use
      * @param handle the handle of the X.509 certificate
      */
      PKCS11_X509_Certificate(Session& session, ObjectHandle handle);

      /**
      * Imports a X.509 certificate
      * @param session the session to use
      * @param props the attributes of the X.509 certificate
      */
      PKCS11_X509_Certificate(Session& session, const X509_CertificateProperties& props);
   };

}
}

#endif

#endif
