/*
* X.509 Certificate Authority
* (C) 1999-2008 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_CA_H_
#define BOTAN_X509_CA_H_

#include <botan/x509cert.h>
#include <botan/x509_crl.h>
#include <chrono>
#include <map>

#if defined(BOTAN_HAS_SYSTEM_RNG)
  #include <botan/system_rng.h>
#endif

namespace Botan {

class BigInt;
class Private_Key;
class PKCS10_Request;
class PK_Signer;

/**
* This class represents X.509 Certificate Authorities (CAs).
*/
class BOTAN_PUBLIC_API(2,0) X509_CA final
   {
   public:
      /**
      * Sign a PKCS#10 Request.
      * @param req the request to sign
      * @param rng the rng to use
      * @param not_before the starting time for the certificate
      * @param not_after the expiration time for the certificate
      * @return resulting certificate
      */
      X509_Certificate sign_request(const PKCS10_Request& req,
                                    RandomNumberGenerator& rng,
                                    const X509_Time& not_before,
                                    const X509_Time& not_after) const;

      /**
      * Sign a PKCS#10 Request.
      * @param req the request to sign
      * @param rng the rng to use
      * @param serial_number the serial number the cert will be assigned.
      * @param not_before the starting time for the certificate
      * @param not_after the expiration time for the certificate
      * @return resulting certificate
      */
      X509_Certificate sign_request(const PKCS10_Request& req,
                                    RandomNumberGenerator& rng,
                                    const BigInt& serial_number,
                                    const X509_Time& not_before,
                                    const X509_Time& not_after) const;

      /**
      * Get the certificate of this CA.
      * @return CA certificate
      */
      X509_Certificate ca_certificate() const;

      /**
      * Create a new and empty CRL for this CA.
      * @param rng the random number generator to use
      * @param issue_time the issue time (typically system_clock::now)
      * @param next_update the time interval after issue_data within which
      *        a new CRL will be produced.
      * @return new CRL
      */
      X509_CRL new_crl(RandomNumberGenerator& rng,
                       std::chrono::system_clock::time_point issue_time,
                       std::chrono::seconds next_update) const;

      /**
      * Create a new CRL by with additional entries.
      * @param last_crl the last CRL of this CA to add the new entries to
      * @param new_entries contains the new CRL entries to be added to the CRL
      * @param rng the random number generator to use
      * @param issue_time the issue time (typically system_clock::now)
      * @param next_update the time interval after issue_data within which
      *        a new CRL will be produced.
      */
      X509_CRL update_crl(const X509_CRL& last_crl,
                          const std::vector<CRL_Entry>& new_entries,
                          RandomNumberGenerator& rng,
                          std::chrono::system_clock::time_point issue_time,
                          std::chrono::seconds next_update) const;

      /**
      * Create a new and empty CRL for this CA.
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      * @return new CRL
      */
      X509_CRL new_crl(RandomNumberGenerator& rng,
                       uint32_t next_update = 604800) const;

      /**
      * Create a new CRL by with additional entries.
      * @param last_crl the last CRL of this CA to add the new entries to
      * @param new_entries contains the new CRL entries to be added to the CRL
      * @param rng the random number generator to use
      * @param next_update the time to set in next update in seconds
      * as the offset from the current time
      */
      X509_CRL update_crl(const X509_CRL& last_crl,
                          const std::vector<CRL_Entry>& new_entries,
                          RandomNumberGenerator& rng,
                          uint32_t next_update = 604800) const;

      /**
      * Interface for creating new certificates
      * @param signer a signing object
      * @param rng a random number generator
      * @param sig_algo the signature algorithm identifier
      * @param pub_key the serialized public key
      * @param not_before the start time of the certificate
      * @param not_after the end time of the certificate
      * @param issuer_dn the DN of the issuer
      * @param subject_dn the DN of the subject
      * @param extensions an optional list of certificate extensions
      * @returns newly minted certificate
      */
      static X509_Certificate make_cert(PK_Signer* signer,
                                        RandomNumberGenerator& rng,
                                        const AlgorithmIdentifier& sig_algo,
                                        const std::vector<uint8_t>& pub_key,
                                        const X509_Time& not_before,
                                        const X509_Time& not_after,
                                        const X509_DN& issuer_dn,
                                        const X509_DN& subject_dn,
                                        const Extensions& extensions);

      /**
      * Interface for creating new certificates
      * @param signer a signing object
      * @param rng a random number generator
      * @param serial_number the serial number the cert will be assigned
      * @param sig_algo the signature algorithm identifier
      * @param pub_key the serialized public key
      * @param not_before the start time of the certificate
      * @param not_after the end time of the certificate
      * @param issuer_dn the DN of the issuer
      * @param subject_dn the DN of the subject
      * @param extensions an optional list of certificate extensions
      * @returns newly minted certificate
      */
      static X509_Certificate make_cert(PK_Signer* signer,
                                        RandomNumberGenerator& rng,
                                        const BigInt& serial_number,
                                        const AlgorithmIdentifier& sig_algo,
                                        const std::vector<uint8_t>& pub_key,
                                        const X509_Time& not_before,
                                        const X509_Time& not_after,
                                        const X509_DN& issuer_dn,
                                        const X509_DN& subject_dn,
                                        const Extensions& extensions);

      /**
      * Create a new CA object.
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param hash_fn name of a hash function to use for signing
      * @param rng the random generator to use
      */
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              const std::string& hash_fn,
              RandomNumberGenerator& rng);

      /**
      * Create a new CA object.
      * @param ca_certificate the certificate of the CA
      * @param key the private key of the CA
      * @param opts additional options, e.g. padding, as key value pairs
      * @param hash_fn name of a hash function to use for signing
      * @param rng the random generator to use
      */
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              const std::map<std::string,std::string>& opts,
              const std::string& hash_fn,
              RandomNumberGenerator& rng);

#if defined(BOTAN_HAS_SYSTEM_RNG)
      BOTAN_DEPRECATED("Use version taking RNG object")
      X509_CA(const X509_Certificate& ca_certificate,
              const Private_Key& key,
              const std::string& hash_fn) :
         X509_CA(ca_certificate, key, hash_fn, system_rng())
         {}
#endif

      X509_CA(const X509_CA&) = delete;
      X509_CA& operator=(const X509_CA&) = delete;

      X509_CA(X509_CA&&) = default;
      X509_CA& operator=(X509_CA&&) = default;

      ~X509_CA();

   private:
      X509_CRL make_crl(const std::vector<CRL_Entry>& entries,
                        uint32_t crl_number,
                        RandomNumberGenerator& rng,
                        std::chrono::system_clock::time_point issue_time,
                        std::chrono::seconds next_update) const;

      AlgorithmIdentifier m_ca_sig_algo;
      X509_Certificate m_ca_cert;
      std::string m_hash_fn;
      std::unique_ptr<PK_Signer> m_signer;
   };

/**
* Choose the default signature format for a certain public key signature
* scheme.
* @param key will be the key to choose a padding scheme for
* @param rng the random generator to use
* @param hash_fn is the desired hash function
* @param alg_id will be set to the chosen scheme
* @return A PK_Signer object for generating signatures
*/
BOTAN_PUBLIC_API(2,0) PK_Signer* choose_sig_format(const Private_Key& key,
                                       RandomNumberGenerator& rng,
                                       const std::string& hash_fn,
                                       AlgorithmIdentifier& alg_id);

/**
* @verbatim
* Choose the default signature format for a certain public key signature
* scheme.
*
* The only option recognized by opts at this moment is "padding"
* Find an entry from src/build-data/oids.txt under [signature] of the form
* <sig_algo>/<padding>[(<hash_algo>)] and add {"padding",<padding>}
* to opts.
* @endverbatim
*
* @param key will be the key to choose a padding scheme for
* @param opts contains additional options for building the certificate
* @param rng the random generator to use
* @param hash_fn is the desired hash function
* @param alg_id will be set to the chosen scheme
* @return A PK_Signer object for generating signatures
*/
PK_Signer* choose_sig_format(const Private_Key& key,
                             const std::map<std::string,std::string>& opts,
                             RandomNumberGenerator& rng,
                             const std::string& hash_fn,
                             AlgorithmIdentifier& alg_id);

}

#endif
