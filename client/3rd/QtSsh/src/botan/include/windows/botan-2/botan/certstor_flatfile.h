/*
* Certificate Store
* (C) 1999-2019 Jack Lloyd
* (C) 2019      Patrick Schmidt
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CERT_STORE_FLATFILE_H_
#define BOTAN_CERT_STORE_FLATFILE_H_

#include <botan/certstor.h>

#include <vector>
#include <memory>
#include <map>

namespace Botan {
/**
* Certificate Store that is backed by a file of PEMs of trusted CAs.
*/
class BOTAN_PUBLIC_API(2, 11) Flatfile_Certificate_Store final : public Certificate_Store
   {
   public:
      /**
      * Construct a new Certificate_Store given a file path to a file including
      * PEMs of trusted self-signed CAs.
      *
      * @param file the name of the file to read certificates from
      * @param ignore_non_ca if true, certs that are not self-signed CA certs will
      * be ignored. Otherwise (if false), an exception will be thrown instead.
      */
      Flatfile_Certificate_Store(const std::string& file, bool ignore_non_ca = false);

      Flatfile_Certificate_Store(const Flatfile_Certificate_Store&) = default;
      Flatfile_Certificate_Store(Flatfile_Certificate_Store&&) = default;
      Flatfile_Certificate_Store& operator=(const Flatfile_Certificate_Store&) = default;
      Flatfile_Certificate_Store& operator=(Flatfile_Certificate_Store&&) = default;

      /**
      * @return DNs for all certificates managed by the store
      */
      std::vector<X509_DN> all_subjects() const override;

      /**
      * Find all certificates with a given Subject DN.
      * Subject DN and even the key identifier might not be unique.
      */
      std::vector<std::shared_ptr<const X509_Certificate>> find_all_certs(
               const X509_DN& subject_dn, const std::vector<uint8_t>& key_id) const override;

      /**
      * Find a certificate by searching for one with a matching SHA-1 hash of
      * public key.
      * @return a matching certificate or nullptr otherwise
      */
      std::shared_ptr<const X509_Certificate>
      find_cert_by_pubkey_sha1(const std::vector<uint8_t>& key_hash) const override;

      std::shared_ptr<const X509_Certificate>
      find_cert_by_raw_subject_dn_sha256(const std::vector<uint8_t>& subject_hash) const override;

      /**
       * Fetching CRLs is not supported by this certificate store. This will
       * always return an empty list.
       */
      std::shared_ptr<const X509_CRL> find_crl_for(const X509_Certificate& subject) const override;

   private:
      std::vector<X509_DN> m_all_subjects;
      std::map<X509_DN, std::vector<std::shared_ptr<const X509_Certificate>>> m_dn_to_cert;
      std::map<std::vector<uint8_t>, std::shared_ptr<const X509_Certificate>> m_pubkey_sha1_to_cert;
      std::map<std::vector<uint8_t>, std::shared_ptr<const X509_Certificate>> m_subject_dn_sha256_to_cert;
   };
}

#endif
