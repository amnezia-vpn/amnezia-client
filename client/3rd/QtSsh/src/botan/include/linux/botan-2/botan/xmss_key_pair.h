/*
 * XMSS Key Pair
 * (C) 2016 Matthias Gierlings
 *
 * Botan is released under the Simplified BSD License (see license.txt)
 **/

#ifndef BOTAN_XMSS_KEY_PAIR_H_
#define BOTAN_XMSS_KEY_PAIR_H_

#include <botan/xmss.h>

BOTAN_DEPRECATED_HEADER(xmss_key_pair.h)

namespace Botan {

/**
 * A pair of XMSS public and private key.
 **/
class BOTAN_PUBLIC_API(2,0) XMSS_Key_Pair
   {
   public:
      XMSS_Key_Pair(XMSS_Parameters::xmss_algorithm_t xmss_oid,
                    RandomNumberGenerator& rng)
         : m_priv_key(xmss_oid, rng), m_pub_key(m_priv_key) {}

      XMSS_Key_Pair(const XMSS_PublicKey& pub_key,
                    const XMSS_PrivateKey& priv_key)
         : m_priv_key(priv_key), m_pub_key(pub_key)
         {}

      XMSS_Key_Pair(XMSS_PublicKey&& pub_key,
                    XMSS_PrivateKey&& priv_key)
         : m_priv_key(std::move(priv_key)), m_pub_key(std::move(pub_key)) {}

      const XMSS_PublicKey& public_key() const { return m_pub_key; }
      XMSS_PublicKey& public_key() { return m_pub_key; }

      const XMSS_PrivateKey& private_key() const { return m_priv_key; }
      XMSS_PrivateKey& private_key() { return m_priv_key; }

   private:
      XMSS_PrivateKey m_priv_key;
      XMSS_PublicKey m_pub_key;
   };

}

#endif
