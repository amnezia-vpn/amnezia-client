/*
* (C) 2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_SYSTEM_CERT_STORE_H_
#define BOTAN_SYSTEM_CERT_STORE_H_

#include <botan/certstor.h>

namespace Botan {

class BOTAN_PUBLIC_API(2,11) System_Certificate_Store final : public Certificate_Store
   {
   public:

      System_Certificate_Store();

      std::shared_ptr<const X509_Certificate>
         find_cert(const X509_DN& subject_dn, const std::vector<uint8_t>& key_id) const override;

      std::vector<std::shared_ptr<const X509_Certificate>>
         find_all_certs(const X509_DN& subject_dn, const std::vector<uint8_t>& key_id) const override;

      std::shared_ptr<const X509_Certificate>
         find_cert_by_pubkey_sha1(const std::vector<uint8_t>& key_hash) const override;

      std::shared_ptr<const X509_Certificate>
         find_cert_by_raw_subject_dn_sha256(const std::vector<uint8_t>& subject_hash) const override;

      std::shared_ptr<const X509_CRL> find_crl_for(const X509_Certificate& subject) const override;

      std::vector<X509_DN> all_subjects() const override;

   private:
      std::shared_ptr<Certificate_Store> m_system_store;
   };

}

#endif
