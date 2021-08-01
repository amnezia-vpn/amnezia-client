/**
 * XMSS WOTS Addressed Private Key
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_WOTS_ADDRESSED_PRIVATEKEY_H_
#define BOTAN_XMSS_WOTS_ADDRESSED_PRIVATEKEY_H_

#include <botan/internal/xmss_address.h>
#include <botan/internal/xmss_wots_addressed_publickey.h>
#include <botan/xmss_wots.h>

namespace Botan {

/**
 * Wrapper class to pair an XMSS_WOTS_PrivateKey with an XMSS Address. Since
 * the PK_Ops::Signature interface does not allow an extra address
 * parameter to be passed to the sign(RandomNumberGenerator&), the address
 * needs to be stored together with the key and passed to the
 * XMSS_WOTS_Signature_Operation() on creation.
 **/
class XMSS_WOTS_Addressed_PrivateKey final :
      public virtual XMSS_WOTS_Addressed_PublicKey,
      public virtual Private_Key
   {
   public:
      XMSS_WOTS_Addressed_PrivateKey(const XMSS_WOTS_PrivateKey& private_key)
         : XMSS_WOTS_Addressed_PublicKey(private_key),
           m_priv_key(private_key) {}

      XMSS_WOTS_Addressed_PrivateKey(const XMSS_WOTS_PrivateKey& private_key,
                                     const XMSS_Address& adrs)
         : XMSS_WOTS_Addressed_PublicKey(private_key, adrs),
           m_priv_key(private_key) {}

      XMSS_WOTS_Addressed_PrivateKey(XMSS_WOTS_PrivateKey&& private_key)
         : XMSS_WOTS_Addressed_PublicKey(XMSS_WOTS_PublicKey(private_key)),
           m_priv_key(std::move(private_key)) {}

      XMSS_WOTS_Addressed_PrivateKey(XMSS_WOTS_PrivateKey&& private_key,
                                     XMSS_Address&& adrs)
         : XMSS_WOTS_Addressed_PublicKey(XMSS_WOTS_PublicKey(private_key),
                                         std::move(adrs)),
           m_priv_key(std::move(private_key)) {}

      const XMSS_WOTS_PrivateKey& private_key() const { return m_priv_key; }
      XMSS_WOTS_PrivateKey& private_key() { return m_priv_key; }

      AlgorithmIdentifier
      pkcs8_algorithm_identifier() const override
         {
         return m_priv_key.pkcs8_algorithm_identifier();
         }

      secure_vector<uint8_t> private_key_bits() const override
         {
         return m_priv_key.private_key_bits();
         }

   private:
      XMSS_WOTS_PrivateKey m_priv_key;
   };

}

#endif
