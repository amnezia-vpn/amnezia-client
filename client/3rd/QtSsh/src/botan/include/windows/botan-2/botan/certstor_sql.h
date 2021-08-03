/*
* Certificate Store in SQL
* (C) 2016 Kai Michaelis, Rohde & Schwarz Cybersecurity
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CERT_STORE_SQL_H_
#define BOTAN_CERT_STORE_SQL_H_

#include <botan/certstor.h>
#include <botan/x509cert.h>
#include <botan/x509_crl.h>
#include <botan/database.h>
#include <botan/mutex.h>

namespace Botan {

class Private_Key;
class RandomNumberGenerator;

/**
 * Certificate and private key store backed by an SQL database.
 */
class BOTAN_PUBLIC_API(2,0) Certificate_Store_In_SQL : public Certificate_Store
   {
   public:
      /**
      * Create/open a certificate store.
      * @param db underlying database storage
      * @param passwd password to encrypt private keys in the database
      * @param rng used for encrypting keys
      * @param table_prefix optional prefix for db table names
      */
      explicit Certificate_Store_In_SQL(const std::shared_ptr<SQL_Database> db,
                                        const std::string& passwd,
                                        RandomNumberGenerator& rng,
                                        const std::string& table_prefix = "");

      /**
      * Returns the first certificate with matching subject DN and optional key ID.
      */
      std::shared_ptr<const X509_Certificate>
         find_cert(const X509_DN& subject_dn, const std::vector<uint8_t>& key_id) const override;

      /*
      * Find all certificates with a given Subject DN.
      * Subject DN and even the key identifier might not be unique.
      */
      std::vector<std::shared_ptr<const X509_Certificate>> find_all_certs(
         const X509_DN& subject_dn, const std::vector<uint8_t>& key_id) const override;

      std::shared_ptr<const X509_Certificate>
         find_cert_by_pubkey_sha1(const std::vector<uint8_t>& key_hash) const override;

      std::shared_ptr<const X509_Certificate>
         find_cert_by_raw_subject_dn_sha256(const std::vector<uint8_t>& subject_hash) const override;

      /**
      * Returns all subject DNs known to the store instance.
      */
      std::vector<X509_DN> all_subjects() const override;

      /**
      * Inserts "cert" into the store, returns false if the certificate is
      * already known and true if insertion was successful.
      */
      bool insert_cert(const X509_Certificate& cert);

      /**
      * Removes "cert" from the store. Returns false if the certificate could not
      * be found and true if removal was successful.
      */
      bool remove_cert(const X509_Certificate& cert);

      /// Returns the private key for "cert" or an empty shared_ptr if none was found.
      std::shared_ptr<const Private_Key> find_key(const X509_Certificate&) const;

      /// Returns all certificates for private key "key".
      std::vector<std::shared_ptr<const X509_Certificate>>
         find_certs_for_key(const Private_Key& key) const;

      /**
      * Inserts "key" for "cert" into the store, returns false if the key is
      * already known and true if insertion was successful.
      */
      bool insert_key(const X509_Certificate& cert, const Private_Key& key);

      /// Removes "key" from the store.
      void remove_key(const Private_Key& key);

      /// Marks "cert" as revoked starting from "time".
      void revoke_cert(const X509_Certificate&, CRL_Code, const X509_Time& time = X509_Time());

      /// Reverses the revokation for "cert".
      void affirm_cert(const X509_Certificate&);

      /**
      * Generates Certificate Revocation Lists for all certificates marked as revoked.
      * A CRL is returned for each unique issuer DN.
      */
      std::vector<X509_CRL> generate_crls() const;

      /**
      * Generates a CRL for all certificates issued by the given issuer.
      */
      std::shared_ptr<const X509_CRL>
         find_crl_for(const X509_Certificate& issuer) const override;

   private:
      RandomNumberGenerator& m_rng;
      std::shared_ptr<SQL_Database> m_database;
      std::string m_prefix;
      std::string m_password;
      mutex_type m_mutex;
   };

}
#endif
