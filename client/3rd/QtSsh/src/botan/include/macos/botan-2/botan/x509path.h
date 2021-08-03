/*
* X.509 Cert Path Validation
* (C) 2010-2011 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_CERT_PATH_VALIDATION_H_
#define BOTAN_X509_CERT_PATH_VALIDATION_H_

#include <botan/pkix_enums.h>
#include <botan/x509cert.h>
#include <botan/certstor.h>
#include <botan/ocsp.h>
#include <functional>
#include <set>
#include <chrono>

#if defined(BOTAN_TARGET_OS_HAS_THREADS) && defined(BOTAN_HAS_HTTP_UTIL)
  #define BOTAN_HAS_ONLINE_REVOCATION_CHECKS
#endif

namespace Botan {

/**
* This type represents the validation status of an entire certificate path.
* There is one set of status codes for each certificate in the path.
*/
typedef std::vector<std::set<Certificate_Status_Code>> CertificatePathStatusCodes;

/**
* Specifies restrictions on the PKIX path validation
*/
class BOTAN_PUBLIC_API(2,0) Path_Validation_Restrictions final
   {
   public:
      /**
      * @param require_rev if true, revocation information is required

      * @param minimum_key_strength is the minimum strength (in terms of
      *    operations, eg 80 means 2^80) of a signature. Signatures weaker than
      *    this are rejected. If more than 80, SHA-1 signatures are also
      *    rejected. If possible use at least setting 110.
      *
      *        80 bit strength requires 1024 bit RSA
      *        110 bit strength requires 2k bit RSA
      *        128 bit strength requires ~3k bit RSA or P-256
      * @param ocsp_all_intermediates Make OCSP requests for all CAs as
      * well as end entity (if OCSP enabled in path validation request)
      * @param max_ocsp_age maximum age of OCSP responses w/o next_update.
      *        If zero, there is no maximum age
      */
      Path_Validation_Restrictions(bool require_rev = false,
                                   size_t minimum_key_strength = 110,
                                   bool ocsp_all_intermediates = false,
                                   std::chrono::seconds max_ocsp_age = std::chrono::seconds::zero());

      /**
      * @param require_rev if true, revocation information is required
      * @param minimum_key_strength is the minimum strength (in terms of
      *        operations, eg 80 means 2^80) of a signature. Signatures
      *        weaker than this are rejected.
      * @param ocsp_all_intermediates Make OCSP requests for all CAs as
      * well as end entity (if OCSP enabled in path validation request)
      * @param trusted_hashes a set of trusted hashes. Any signatures
      *        created using a hash other than one of these will be
      *        rejected.
      * @param max_ocsp_age maximum age of OCSP responses w/o next_update.
      *        If zero, there is no maximum age
      */
      Path_Validation_Restrictions(bool require_rev,
                                   size_t minimum_key_strength,
                                   bool ocsp_all_intermediates,
                                   const std::set<std::string>& trusted_hashes,
                                   std::chrono::seconds max_ocsp_age = std::chrono::seconds::zero()) :
         m_require_revocation_information(require_rev),
         m_ocsp_all_intermediates(ocsp_all_intermediates),
         m_trusted_hashes(trusted_hashes),
         m_minimum_key_strength(minimum_key_strength),
         m_max_ocsp_age(max_ocsp_age) {}

      /**
      * @return whether revocation information is required
      */
      bool require_revocation_information() const
         { return m_require_revocation_information; }

      /**
      * @return whether all intermediate CAs should also be OCSPed. If false
      * then only end entity OCSP is required/requested.
      */
      bool ocsp_all_intermediates() const
         { return m_ocsp_all_intermediates; }

      /**
      * @return trusted signature hash functions
      */
      const std::set<std::string>& trusted_hashes() const
         { return m_trusted_hashes; }

      /**
      * @return minimum required key strength
      */
      size_t minimum_key_strength() const
         { return m_minimum_key_strength; }

      /**
      * @return maximum age of OCSP responses w/o next_update.
      * If zero, there is no maximum age
      */
      std::chrono::seconds max_ocsp_age() const
         { return m_max_ocsp_age; }

   private:
      bool m_require_revocation_information;
      bool m_ocsp_all_intermediates;
      std::set<std::string> m_trusted_hashes;
      size_t m_minimum_key_strength;
      std::chrono::seconds m_max_ocsp_age;
   };

/**
* Represents the result of a PKIX path validation
*/
class BOTAN_PUBLIC_API(2,0) Path_Validation_Result final
   {
   public:
      typedef Certificate_Status_Code Code;

      /**
      * @return the set of hash functions you are implicitly
      * trusting by trusting this result.
      */
      std::set<std::string> trusted_hashes() const;

      /**
      * @return the trust root of the validation if successful
      * throws an exception if the validation failed
      */
      const X509_Certificate& trust_root() const;

      /**
      * @return the full path from subject to trust root
      * This path may be empty
      */
      const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path() const { return m_cert_path; }

      /**
      * @return true iff the validation was successful
      */
      bool successful_validation() const;

      /**
      * @return true iff no warnings occured during validation
      */
      bool no_warnings() const;

      /**
      * @return overall validation result code
      */
      Certificate_Status_Code result() const { return m_overall; }

      /**
      * @return a set of status codes for each certificate in the chain
      */
      const CertificatePathStatusCodes& all_statuses() const
         { return m_all_status; }

      /**
      * @return the subset of status codes that are warnings
      */
      CertificatePathStatusCodes warnings() const;

      /**
      * @return string representation of the validation result
      */
      std::string result_string() const;

      /**
      * @return string representation of the warnings
      */
      std::string warnings_string() const;

      /**
      * @param code validation status code
      * @return corresponding validation status message
      */
      static const char* status_string(Certificate_Status_Code code);

      /**
      * Create a Path_Validation_Result
      * @param status list of validation status codes
      * @param cert_chain the certificate chain that was validated
      */
      Path_Validation_Result(CertificatePathStatusCodes status,
                             std::vector<std::shared_ptr<const X509_Certificate>>&& cert_chain);

      /**
      * Create a Path_Validation_Result
      * @param status validation status code
      */
      explicit Path_Validation_Result(Certificate_Status_Code status) : m_overall(status) {}

   private:
      CertificatePathStatusCodes m_all_status;
      CertificatePathStatusCodes m_warnings;
      std::vector<std::shared_ptr<const X509_Certificate>> m_cert_path;
      Certificate_Status_Code m_overall;
   };

/**
* PKIX Path Validation
* @param end_certs certificate chain to validate (with end entity certificate in end_certs[0])
* @param restrictions path validation restrictions
* @param trusted_roots list of certificate stores that contain trusted certificates
* @param hostname if not empty, compared against the DNS name in end_certs[0]
* @param usage if not set to UNSPECIFIED, compared against the key usage in end_certs[0]
* @param validation_time what reference time to use for validation
* @param ocsp_timeout timeout for OCSP operations, 0 disables OCSP check
* @param ocsp_resp additional OCSP responses to consider (eg from peer)
* @return result of the path validation
*   note: when enabled, OCSP check is softfail by default: if the OCSP server is not
*   reachable, Path_Validation_Result::successful_validation() will return true.
*   Hardfail OCSP check can be achieve by also calling Path_Validation_Result::no_warnings().
*/
Path_Validation_Result BOTAN_PUBLIC_API(2,0) x509_path_validate(
   const std::vector<X509_Certificate>& end_certs,
   const Path_Validation_Restrictions& restrictions,
   const std::vector<Certificate_Store*>& trusted_roots,
   const std::string& hostname = "",
   Usage_Type usage = Usage_Type::UNSPECIFIED,
   std::chrono::system_clock::time_point validation_time = std::chrono::system_clock::now(),
   std::chrono::milliseconds ocsp_timeout = std::chrono::milliseconds(0),
   const std::vector<std::shared_ptr<const OCSP::Response>>& ocsp_resp = {});

/**
* PKIX Path Validation
* @param end_cert certificate to validate
* @param restrictions path validation restrictions
* @param trusted_roots list of stores that contain trusted certificates
* @param hostname if not empty, compared against the DNS name in end_cert
* @param usage if not set to UNSPECIFIED, compared against the key usage in end_cert
* @param validation_time what reference time to use for validation
* @param ocsp_timeout timeout for OCSP operations, 0 disables OCSP check
* @param ocsp_resp additional OCSP responses to consider (eg from peer)
* @return result of the path validation
*/
Path_Validation_Result BOTAN_PUBLIC_API(2,0) x509_path_validate(
   const X509_Certificate& end_cert,
   const Path_Validation_Restrictions& restrictions,
   const std::vector<Certificate_Store*>& trusted_roots,
   const std::string& hostname = "",
   Usage_Type usage = Usage_Type::UNSPECIFIED,
   std::chrono::system_clock::time_point validation_time = std::chrono::system_clock::now(),
   std::chrono::milliseconds ocsp_timeout = std::chrono::milliseconds(0),
   const std::vector<std::shared_ptr<const OCSP::Response>>& ocsp_resp = {});

/**
* PKIX Path Validation
* @param end_cert certificate to validate
* @param restrictions path validation restrictions
* @param store store that contains trusted certificates
* @param hostname if not empty, compared against the DNS name in end_cert
* @param usage if not set to UNSPECIFIED, compared against the key usage in end_cert
* @param validation_time what reference time to use for validation
* @param ocsp_timeout timeout for OCSP operations, 0 disables OCSP check
* @param ocsp_resp additional OCSP responses to consider (eg from peer)
* @return result of the path validation
*/
Path_Validation_Result BOTAN_PUBLIC_API(2,0) x509_path_validate(
   const X509_Certificate& end_cert,
   const Path_Validation_Restrictions& restrictions,
   const Certificate_Store& store,
   const std::string& hostname = "",
   Usage_Type usage = Usage_Type::UNSPECIFIED,
   std::chrono::system_clock::time_point validation_time = std::chrono::system_clock::now(),
   std::chrono::milliseconds ocsp_timeout = std::chrono::milliseconds(0),
   const std::vector<std::shared_ptr<const OCSP::Response>>& ocsp_resp = {});

/**
* PKIX Path Validation
* @param end_certs certificate chain to validate
* @param restrictions path validation restrictions
* @param store store that contains trusted certificates
* @param hostname if not empty, compared against the DNS name in end_certs[0]
* @param usage if not set to UNSPECIFIED, compared against the key usage in end_certs[0]
* @param validation_time what reference time to use for validation
* @param ocsp_timeout timeout for OCSP operations, 0 disables OCSP check
* @param ocsp_resp additional OCSP responses to consider (eg from peer)
* @return result of the path validation
*/
Path_Validation_Result BOTAN_PUBLIC_API(2,0) x509_path_validate(
   const std::vector<X509_Certificate>& end_certs,
   const Path_Validation_Restrictions& restrictions,
   const Certificate_Store& store,
   const std::string& hostname = "",
   Usage_Type usage = Usage_Type::UNSPECIFIED,
   std::chrono::system_clock::time_point validation_time = std::chrono::system_clock::now(),
   std::chrono::milliseconds ocsp_timeout = std::chrono::milliseconds(0),
   const std::vector<std::shared_ptr<const OCSP::Response>>& ocsp_resp = {});


/**
* namespace PKIX holds the building blocks that are called by x509_path_validate.
* This allows custom validation logic to be written by applications and makes
* for easier testing, but unless you're positive you know what you're doing you
* probably want to just call x509_path_validate instead.
*/
namespace PKIX {

Certificate_Status_Code
build_all_certificate_paths(std::vector<std::vector<std::shared_ptr<const X509_Certificate>>>& cert_paths,
                            const std::vector<Certificate_Store*>& trusted_certstores,
                            const std::shared_ptr<const X509_Certificate>& end_entity,
                            const std::vector<std::shared_ptr<const X509_Certificate>>& end_entity_extra);


/**
* Build certificate path
* @param cert_path_out output parameter, cert_path will be appended to this vector
* @param trusted_certstores list of certificate stores that contain trusted certificates
* @param end_entity the cert to be validated
* @param end_entity_extra optional list of additional untrusted certs for path building
* @return result of the path building operation (OK or error)
*/
Certificate_Status_Code
BOTAN_PUBLIC_API(2,0) build_certificate_path(std::vector<std::shared_ptr<const X509_Certificate>>& cert_path_out,
                                 const std::vector<Certificate_Store*>& trusted_certstores,
                                 const std::shared_ptr<const X509_Certificate>& end_entity,
                                 const std::vector<std::shared_ptr<const X509_Certificate>>& end_entity_extra);

/**
* Check the certificate chain, but not any revocation data
*
* @param cert_path path built by build_certificate_path with OK result
* @param ref_time whatever time you want to perform the validation
* against (normally current system clock)
* @param hostname the hostname
* @param usage end entity usage checks
* @param min_signature_algo_strength 80 or 110 typically
* Note 80 allows 1024 bit RSA and SHA-1. 110 allows 2048 bit RSA and SHA-2.
* Using 128 requires ECC (P-256) or ~3000 bit RSA keys.
* @param trusted_hashes set of trusted hash functions, empty means accept any
* hash we have an OID for
* @return vector of results on per certificate in the path, each containing a set of
* results. If all codes in the set are < Certificate_Status_Code::FIRST_ERROR_STATUS,
* then the result for that certificate is successful. If all results are
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2,0) check_chain(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
                      std::chrono::system_clock::time_point ref_time,
                      const std::string& hostname,
                      Usage_Type usage,
                      size_t min_signature_algo_strength,
                      const std::set<std::string>& trusted_hashes);

/**
* Check OCSP responses for revocation information
* @param cert_path path already validated by check_chain
* @param ocsp_responses the OCSP responses to consider
* @param certstores trusted roots
* @param ref_time whatever time you want to perform the validation against
* (normally current system clock)
* @param max_ocsp_age maximum age of OCSP responses w/o next_update. If zero,
* there is no maximum age
* @return revocation status
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2, 0) check_ocsp(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
                                  const std::vector<std::shared_ptr<const OCSP::Response>>& ocsp_responses,
                                  const std::vector<Certificate_Store*>& certstores,
                                  std::chrono::system_clock::time_point ref_time,
                                  std::chrono::seconds max_ocsp_age = std::chrono::seconds::zero());

/**
* Check CRLs for revocation information
* @param cert_path path already validated by check_chain
* @param crls the list of CRLs to check, it is assumed that crls[i] (if not null)
* is the associated CRL for the subject in cert_path[i].
* @param ref_time whatever time you want to perform the validation against
* (normally current system clock)
* @return revocation status
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2,0) check_crl(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
                    const std::vector<std::shared_ptr<const X509_CRL>>& crls,
                    std::chrono::system_clock::time_point ref_time);

/**
* Check CRLs for revocation information
* @param cert_path path already validated by check_chain
* @param certstores a list of certificate stores to query for the CRL
* @param ref_time whatever time you want to perform the validation against
* (normally current system clock)
* @return revocation status
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2,0) check_crl(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
                    const std::vector<Certificate_Store*>& certstores,
                    std::chrono::system_clock::time_point ref_time);

#if defined(BOTAN_HAS_ONLINE_REVOCATION_CHECKS)

/**
* Check OCSP using online (HTTP) access. Current version creates a thread and
* network connection per OCSP request made.
*
* @param cert_path path already validated by check_chain
* @param trusted_certstores a list of certstores with trusted certs
* @param ref_time whatever time you want to perform the validation against
* (normally current system clock)
* @param timeout for timing out the responses, though actually this function
* may block for up to timeout*cert_path.size()*C for some small C.
* @param ocsp_check_intermediate_CAs if true also performs OCSP on any intermediate
* CA certificates. If false, only does OCSP on the end entity cert.
* @param max_ocsp_age maximum age of OCSP responses w/o next_update. If zero,
* there is no maximum age
* @return revocation status
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2, 0) check_ocsp_online(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
      const std::vector<Certificate_Store*>& trusted_certstores,
      std::chrono::system_clock::time_point ref_time,
      std::chrono::milliseconds timeout,
      bool ocsp_check_intermediate_CAs,
      std::chrono::seconds max_ocsp_age = std::chrono::seconds::zero());

/**
* Check CRL using online (HTTP) access. Current version creates a thread and
* network connection per CRL access.

* @param cert_path path already validated by check_chain
* @param trusted_certstores a list of certstores with trusted certs
* @param certstore_to_recv_crls optional (nullptr to disable), all CRLs
* retreived will be saved to this cert store.
* @param ref_time whatever time you want to perform the validation against
* (normally current system clock)
* @param timeout for timing out the responses, though actually this function
* may block for up to timeout*cert_path.size()*C for some small C.
* @return revocation status
*/
CertificatePathStatusCodes
BOTAN_PUBLIC_API(2,0) check_crl_online(const std::vector<std::shared_ptr<const X509_Certificate>>& cert_path,
                           const std::vector<Certificate_Store*>& trusted_certstores,
                           Certificate_Store_In_Memory* certstore_to_recv_crls,
                           std::chrono::system_clock::time_point ref_time,
                           std::chrono::milliseconds timeout);

#endif

/**
* Find overall status (OK, error) of a validation
* @param cert_status result of merge_revocation_status or check_chain
*/
Certificate_Status_Code BOTAN_PUBLIC_API(2,0) overall_status(const CertificatePathStatusCodes& cert_status);

/**
* Merge the results from CRL and/or OCSP checks into chain_status
* @param chain_status the certificate status
* @param crl_status results from check_crl
* @param ocsp_status results from check_ocsp
* @param require_rev_on_end_entity require valid CRL or OCSP on end-entity cert
* @param require_rev_on_intermediates require valid CRL or OCSP on all intermediate certificates
*/
void BOTAN_PUBLIC_API(2,0) merge_revocation_status(CertificatePathStatusCodes& chain_status,
                                       const CertificatePathStatusCodes& crl_status,
                                       const CertificatePathStatusCodes& ocsp_status,
                                       bool require_rev_on_end_entity,
                                       bool require_rev_on_intermediates);

}

}

#endif
