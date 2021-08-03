/**
 * XMSS WOTS Addressed Public Key
 * (C) 2016,2017 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/


#ifndef BOTAN_XMSS_WOTS_ADDRESSED_PUBLICKEY_H_
#define BOTAN_XMSS_WOTS_ADDRESSED_PUBLICKEY_H_

#include <botan/internal/xmss_address.h>
#include <botan/xmss_wots.h>

namespace Botan {

/**
 * Wrapper class to pair a XMSS_WOTS_PublicKey with an XMSS Address. Since
 * the PK_Ops::Verification interface does not allow an extra address
 * parameter to be passed to the sign(RandomNumberGenerator&), the address
 * needs to be stored together with the key and passed to the
 * XMSS_WOTS_Verification_Operation() on creation.
 **/
class XMSS_WOTS_Addressed_PublicKey : public virtual Public_Key
   {
   public:
      XMSS_WOTS_Addressed_PublicKey(const XMSS_WOTS_PublicKey& public_key)
         : m_pub_key(public_key), m_adrs() {}

      XMSS_WOTS_Addressed_PublicKey(const XMSS_WOTS_PublicKey& public_key,
                                    const XMSS_Address& adrs)
         : m_pub_key(public_key), m_adrs(adrs) {}

      XMSS_WOTS_Addressed_PublicKey(XMSS_WOTS_PublicKey&& public_key)
         : m_pub_key(std::move(public_key)), m_adrs() {}

      XMSS_WOTS_Addressed_PublicKey(XMSS_WOTS_PublicKey&& public_key,
                                    XMSS_Address&& adrs)
         : m_pub_key(std::move(public_key)), m_adrs(std::move(adrs)) {}

      const XMSS_WOTS_PublicKey& public_key() const { return m_pub_key; }
      XMSS_WOTS_PublicKey& public_key()  { return m_pub_key; }

      const XMSS_Address& address() const { return m_adrs; }
      XMSS_Address& address() { return m_adrs; }

      std::string algo_name() const override
         {
         return m_pub_key.algo_name();
         }

      AlgorithmIdentifier algorithm_identifier() const override
         {
         return m_pub_key.algorithm_identifier();
         }

      bool check_key(RandomNumberGenerator& rng, bool strong) const override
         {
         return m_pub_key.check_key(rng, strong);
         }

      std::unique_ptr<PK_Ops::Verification>
      create_verification_op(const std::string& params,
                             const std::string& provider) const override
         {
         return m_pub_key.create_verification_op(params, provider);
         }

      OID get_oid() const override
         {
         return m_pub_key.get_oid();
         }

      size_t estimated_strength() const override
         {
         return m_pub_key.estimated_strength();
         }

      size_t key_length() const override
         {
         return m_pub_key.estimated_strength();
         }

      std::vector<uint8_t> public_key_bits() const override
         {
         return m_pub_key.public_key_bits();
         }

   protected:
      XMSS_WOTS_PublicKey m_pub_key;
      XMSS_Address m_adrs;
   };

}

#endif
