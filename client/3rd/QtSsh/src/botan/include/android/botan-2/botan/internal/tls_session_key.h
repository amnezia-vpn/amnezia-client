/*
* TLS Session Key
* (C) 2004-2006,2011 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SESSION_KEYS_H_
#define BOTAN_TLS_SESSION_KEYS_H_

#include <botan/secmem.h>
#include <botan/tls_magic.h>

namespace Botan {

namespace TLS {

class Handshake_State;

/**
* TLS Session Keys
*/
class Session_Keys final
   {
   public:
      /**
      * @return client AEAD key
      */
      const secure_vector<uint8_t>& client_aead_key() const { return m_c_aead; }

      /**
      * @return server AEAD key
      */
      const secure_vector<uint8_t>& server_aead_key() const { return m_s_aead; }

      /**
      * @return client nonce
      */
      const std::vector<uint8_t>& client_nonce() const { return m_c_nonce; }

      /**
      * @return server nonce
      */
      const std::vector<uint8_t>& server_nonce() const { return m_s_nonce; }

      /**
      * @return TLS master secret
      */
      const secure_vector<uint8_t>& master_secret() const { return m_master_sec; }

      const secure_vector<uint8_t>& aead_key(Connection_Side side) const
         {
         return (side == Connection_Side::CLIENT) ? client_aead_key() : server_aead_key();
         }

      const std::vector<uint8_t>& nonce(Connection_Side side) const
         {
         return (side == Connection_Side::CLIENT) ? client_nonce() : server_nonce();
         }

      Session_Keys() = default;

      /**
      * @param state state the handshake state
      * @param pre_master_secret the pre-master secret
      * @param resuming whether this TLS session is resumed
      */
      Session_Keys(const Handshake_State* state,
                   const secure_vector<uint8_t>& pre_master_secret,
                   bool resuming);

   private:
      secure_vector<uint8_t> m_master_sec;
      secure_vector<uint8_t> m_c_aead, m_s_aead;
      std::vector<uint8_t> m_c_nonce, m_s_nonce;
   };

}

}

#endif
