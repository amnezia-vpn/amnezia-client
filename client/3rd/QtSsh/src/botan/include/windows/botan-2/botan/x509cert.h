/*
* X.509 Certificates
* (C) 1999-2007,2015,2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_CERTS_H_
#define BOTAN_X509_CERTS_H_

#include <botan/x509_obj.h>
#include <memory>

namespace Botan {

class Public_Key;
class X509_DN;
class Extensions;
class AlternativeName;
class NameConstraints;

enum class Usage_Type
   {
   UNSPECIFIED, // no restrictions
   TLS_SERVER_AUTH,
   TLS_CLIENT_AUTH,
   CERTIFICATE_AUTHORITY,
   OCSP_RESPONDER,
   ENCRYPTION
   };

struct X509_Certificate_Data;

/**
* This class represents an X.509 Certificate
*/
class BOTAN_PUBLIC_API(2,0) X509_Certificate : public X509_Object
   {
   public:
      /**
      * Return a newly allocated copy of the public key associated
      * with the subject of this certificate. This object is owned
      * by the caller.
      *
      * Prefer load_subject_public_key in new code
      *
      * @return public key
      */
      Public_Key* subject_public_key() const;

      /**
      * Create a public key object associated with the public key bits in this
      * certificate. If the public key bits was valid for X.509 encoding
      * purposes but invalid algorithmically (for example, RSA with an even
      * modulus) that will be detected at this point, and an exception will be
      * thrown.
      *
      * @return subject public key of this certificate
      */
      std::unique_ptr<Public_Key> load_subject_public_key() const;

      /**
      * Get the public key associated with this certificate. This includes the
      * outer AlgorithmIdentifier
      * @return subject public key of this certificate
      */
      const std::vector<uint8_t>& subject_public_key_bits() const;

      /**
      * Get the SubjectPublicKeyInfo associated with this certificate.
      * @return subject public key info of this certificate
      */
      const std::vector<uint8_t>& subject_public_key_info() const;

      /**
      * Return the algorithm identifier of the public key
      */
      const AlgorithmIdentifier& subject_public_key_algo() const;

      /**
      * Get the bit string of the public key associated with this certificate
      * @return public key bits
      */
      const std::vector<uint8_t>& subject_public_key_bitstring() const;

      /**
      * Get the SHA-1 bit string of the public key associated with this certificate.
      * This is used for OCSP among other protocols.
      * This function will throw if SHA-1 is not available.
      * @return hash of subject public key of this certificate
      */
      const std::vector<uint8_t>& subject_public_key_bitstring_sha1() const;

      /**
      * Get the certificate's issuer distinguished name (DN).
      * @return issuer DN of this certificate
      */
      const X509_DN& issuer_dn() const;

      /**
      * Get the certificate's subject distinguished name (DN).
      * @return subject DN of this certificate
      */
      const X509_DN& subject_dn() const;

      /**
      * Get a value for a specific subject_info parameter name.
      * @param name the name of the parameter to look up. Possible names include
      * "X509.Certificate.version", "X509.Certificate.serial",
      * "X509.Certificate.start", "X509.Certificate.end",
      * "X509.Certificate.v2.key_id", "X509.Certificate.public_key",
      * "X509v3.BasicConstraints.path_constraint",
      * "X509v3.BasicConstraints.is_ca", "X509v3.NameConstraints",
      * "X509v3.ExtendedKeyUsage", "X509v3.CertificatePolicies",
      * "X509v3.SubjectKeyIdentifier", "X509.Certificate.serial",
      * "X520.CommonName", "X520.Organization", "X520.Country",
      * "RFC822" (Email in SAN) or "PKCS9.EmailAddress" (Email in DN).
      * @return value(s) of the specified parameter
      */
      std::vector<std::string> subject_info(const std::string& name) const;

      /**
      * Get a value for a specific subject_info parameter name.
      * @param name the name of the parameter to look up. Possible names are
      * "X509.Certificate.v2.key_id" or "X509v3.AuthorityKeyIdentifier".
      * @return value(s) of the specified parameter
      */
      std::vector<std::string> issuer_info(const std::string& name) const;

      /**
      * Raw issuer DN bits
      */
      const std::vector<uint8_t>& raw_issuer_dn() const;

      /**
      * SHA-256 of Raw issuer DN
      */
      std::vector<uint8_t> raw_issuer_dn_sha256() const;

      /**
      * Raw subject DN
      */
      const std::vector<uint8_t>& raw_subject_dn() const;

      /**
      * SHA-256 of Raw subject DN
      */
      std::vector<uint8_t> raw_subject_dn_sha256() const;

      /**
      * Get the notBefore of the certificate as a string
      * @return notBefore of the certificate
      */
      std::string BOTAN_DEPRECATED("Use not_before().to_string()") start_time() const
         {
         return not_before().to_string();
         }

      /**
      * Get the notAfter of the certificate as a string
      * @return notAfter of the certificate
      */
      std::string BOTAN_DEPRECATED("Use not_after().to_string()") end_time() const
         {
         return not_after().to_string();
         }

      /**
      * Get the notBefore of the certificate as X509_Time
      * @return notBefore of the certificate
      */
      const X509_Time& not_before() const;

      /**
      * Get the notAfter of the certificate as X509_Time
      * @return notAfter of the certificate
      */
      const X509_Time& not_after() const;

      /**
      * Get the X509 version of this certificate object.
      * @return X509 version
      */
      uint32_t x509_version() const;

      /**
      * Get the serial number of this certificate.
      * @return certificates serial number
      */
      const std::vector<uint8_t>& serial_number() const;

      /**
      * Get the serial number's sign
      * @return 1 iff the serial is negative.
      */
      bool is_serial_negative() const;

      /**
      * Get the DER encoded AuthorityKeyIdentifier of this certificate.
      * @return DER encoded AuthorityKeyIdentifier
      */
      const std::vector<uint8_t>& authority_key_id() const;

      /**
      * Get the DER encoded SubjectKeyIdentifier of this certificate.
      * @return DER encoded SubjectKeyIdentifier
      */
      const std::vector<uint8_t>& subject_key_id() const;

      /**
      * Check whether this certificate is self signed.
      * If the DN issuer and subject agree,
      * @return true if this certificate is self signed
      */
      bool is_self_signed() const;

      /**
      * Check whether this certificate is a CA certificate.
      * @return true if this certificate is a CA certificate
      */
      bool is_CA_cert() const;

      /**
      * Returns true if the specified @param usage is set in the key usage extension
      * or if no key usage constraints are set at all.
      * To check if a certain key constraint is set in the certificate
      * use @see X509_Certificate#has_constraints.
      */
      bool allowed_usage(Key_Constraints usage) const;

      /**
      * Returns true if the specified @param usage is set in the extended key usage extension
      * or if no extended key usage constraints are set at all.
      * To check if a certain extended key constraint is set in the certificate
      * use @see X509_Certificate#has_ex_constraint.
      */
      bool allowed_extended_usage(const std::string& usage) const;

      /**
      * Returns true if the specified usage is set in the extended key usage extension,
      * or if no extended key usage constraints are set at all.
      * To check if a certain extended key constraint is set in the certificate
      * use @see X509_Certificate#has_ex_constraint.
      */
      bool allowed_extended_usage(const OID& usage) const;

      /**
      * Returns true if the required key and extended key constraints are set in the certificate
      * for the specified @param usage or if no key constraints are set in both the key usage
      * and extended key usage extension.
      */
      bool allowed_usage(Usage_Type usage) const;

      /**
      * Returns true if the specified @param constraints are included in the key
      * usage extension.
      */
      bool has_constraints(Key_Constraints constraints) const;

      /**
      * Returns true if and only if @param ex_constraint (referring to an
      * extended key constraint, eg "PKIX.ServerAuth") is included in the
      * extended key extension.
      */
      bool BOTAN_DEPRECATED("Use version taking an OID")
         has_ex_constraint(const std::string& ex_constraint) const;

      /**
      * Returns true if and only if OID @param ex_constraint is
      * included in the extended key extension.
      */
      bool has_ex_constraint(const OID& ex_constraint) const;

      /**
      * Get the path limit as defined in the BasicConstraints extension of
      * this certificate.
      * @return path limit
      */
      uint32_t path_limit() const;

      /**
      * Check whenever a given X509 Extension is marked critical in this
      * certificate.
      */
      bool is_critical(const std::string& ex_name) const;

      /**
      * Get the key constraints as defined in the KeyUsage extension of this
      * certificate.
      * @return key constraints
      */
      Key_Constraints constraints() const;

      /**
      * Get the key constraints as defined in the ExtendedKeyUsage
      * extension of this certificate.
      * @return key constraints
      */
      std::vector<std::string>
         BOTAN_DEPRECATED("Use extended_key_usage") ex_constraints() const;

      /**
      * Get the key usage as defined in the ExtendedKeyUsage extension
      * of this certificate, or else an empty vector.
      * @return key usage
      */
      const std::vector<OID>& extended_key_usage() const;

      /**
      * Get the name constraints as defined in the NameConstraints
      * extension of this certificate.
      * @return name constraints
      */
      const NameConstraints& name_constraints() const;

      /**
      * Get the policies as defined in the CertificatePolicies extension
      * of this certificate.
      * @return certificate policies
      */
      std::vector<std::string> BOTAN_DEPRECATED("Use certificate_policy_oids") policies() const;

      const std::vector<OID>& certificate_policy_oids() const;

      /**
      * Get all extensions of this certificate.
      * @return certificate extensions
      */
      const Extensions& v3_extensions() const;

      /**
      * Return the v2 issuer key ID. v2 key IDs are almost never used,
      * instead see v3_subject_key_id.
      */
      const std::vector<uint8_t>& v2_issuer_key_id() const;

      /**
      * Return the v2 subject key ID. v2 key IDs are almost never used,
      * instead see v3_subject_key_id.
      */
      const std::vector<uint8_t>& v2_subject_key_id() const;

      /**
      * Return the subject alternative names (DNS, IP, ...)
      */
      const AlternativeName& subject_alt_name() const;

      /**
      * Return the issuer alternative names (DNS, IP, ...)
      */
      const AlternativeName& issuer_alt_name() const;

      /**
      * Return the listed address of an OCSP responder, or empty if not set
      */
      std::string ocsp_responder() const;

      /**
      * Return the listed addresses of ca issuers, or empty if not set
      */
      std::vector<std::string> ca_issuers() const;

      /**
      * Return the CRL distribution point, or empty if not set
      */
      std::string crl_distribution_point() const;

      /**
      * @return a free-form string describing the certificate
      */
      std::string to_string() const;

      /**
      * @return a fingerprint of the certificate
      * @param hash_name hash function used to calculate the fingerprint
      */
      std::string fingerprint(const std::string& hash_name = "SHA-1") const;

      /**
      * Check if a certain DNS name matches up with the information in
      * the cert
      * @param name DNS name to match
      */
      bool matches_dns_name(const std::string& name) const;

      /**
      * Check to certificates for equality.
      * @return true both certificates are (binary) equal
      */
      bool operator==(const X509_Certificate& other) const;

      /**
      * Impose an arbitrary (but consistent) ordering, eg to allow sorting
      * a container of certificate objects.
      * @return true if this is less than other by some unspecified criteria
      */
      bool operator<(const X509_Certificate& other) const;

      /**
      * Create a certificate from a data source providing the DER or
      * PEM encoded certificate.
      * @param source the data source
      */
      explicit X509_Certificate(DataSource& source);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
      /**
      * Create a certificate from a file containing the DER or PEM
      * encoded certificate.
      * @param filename the name of the certificate file
      */
      explicit X509_Certificate(const std::string& filename);
#endif

      /**
      * Create a certificate from a buffer
      * @param in the buffer containing the DER-encoded certificate
      */
      explicit X509_Certificate(const std::vector<uint8_t>& in);

      /**
      * Create a certificate from a buffer
      * @param data the buffer containing the DER-encoded certificate
      * @param length length of data in bytes
      */
      X509_Certificate(const uint8_t data[], size_t length);

      /**
      * Create an uninitialized certificate object. Any attempts to
      * access this object will throw an exception.
      */
      X509_Certificate() = default;

      X509_Certificate(const X509_Certificate& other) = default;

      X509_Certificate& operator=(const X509_Certificate& other) = default;

   private:
      std::string PEM_label() const override;

      std::vector<std::string> alternate_PEM_labels() const override;

      void force_decode() override;

      const X509_Certificate_Data& data() const;

      std::shared_ptr<X509_Certificate_Data> m_data;
   };

/**
* Check two certificates for inequality
* @param cert1 The first certificate
* @param cert2 The second certificate
* @return true if the arguments represent different certificates,
* false if they are binary identical
*/
BOTAN_PUBLIC_API(2,0) bool operator!=(const X509_Certificate& cert1, const X509_Certificate& cert2);

}

#endif
