/*
* TLS Session
* (C) 2011-2012,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SESSION_STATE_H_
#define BOTAN_TLS_SESSION_STATE_H_

#include <botan/x509cert.h>
#include <botan/tls_version.h>
#include <botan/tls_ciphersuite.h>
#include <botan/tls_magic.h>
#include <botan/tls_server_info.h>
#include <botan/secmem.h>
#include <botan/symkey.h>
#include <chrono>

namespace Botan {

namespace TLS {

/**
* Class representing a TLS session state
*/
class BOTAN_PUBLIC_API(2,0) Session final
   {
   public:

      /**
      * Uninitialized session
      */
      Session() :
         m_start_time(std::chrono::system_clock::time_point::min()),
         m_version(),
         m_ciphersuite(0),
         m_connection_side(static_cast<Connection_Side>(0)),
         m_srtp_profile(0),
         m_extended_master_secret(false),
         m_encrypt_then_mac(false)
            {}

      /**
      * New session (sets session start time)
      */
      Session(const std::vector<uint8_t>& session_id,
              const secure_vector<uint8_t>& master_secret,
              Protocol_Version version,
              uint16_t ciphersuite,
              Connection_Side side,
              bool supports_extended_master_secret,
              bool supports_encrypt_then_mac,
              const std::vector<X509_Certificate>& peer_certs,
              const std::vector<uint8_t>& session_ticket,
              const Server_Information& server_info,
              const std::string& srp_identifier,
              uint16_t srtp_profile);

      /**
      * Load a session from DER representation (created by DER_encode)
      * @param ber DER representation buffer
      * @param ber_len size of buffer in bytes
      */
      Session(const uint8_t ber[], size_t ber_len);

      /**
      * Load a session from PEM representation (created by PEM_encode)
      * @param pem PEM representation
      */
      explicit Session(const std::string& pem);

      /**
      * Encode this session data for storage
      * @warning if the master secret is compromised so is the
      * session traffic
      */
      secure_vector<uint8_t> DER_encode() const;

      /**
      * Encrypt a session (useful for serialization or session tickets)
      */
      std::vector<uint8_t> encrypt(const SymmetricKey& key,
                                RandomNumberGenerator& rng) const;


      /**
      * Decrypt a session created by encrypt
      * @param ctext the ciphertext returned by encrypt
      * @param ctext_size the size of ctext in bytes
      * @param key the same key used by the encrypting side
      */
      static Session decrypt(const uint8_t ctext[],
                             size_t ctext_size,
                             const SymmetricKey& key);

      /**
      * Decrypt a session created by encrypt
      * @param ctext the ciphertext returned by encrypt
      * @param key the same key used by the encrypting side
      */
      static inline Session decrypt(const std::vector<uint8_t>& ctext,
                                    const SymmetricKey& key)
         {
         return Session::decrypt(ctext.data(), ctext.size(), key);
         }

      /**
      * Encode this session data for storage
      * @warning if the master secret is compromised so is the
      * session traffic
      */
      std::string PEM_encode() const;

      /**
      * Get the version of the saved session
      */
      Protocol_Version version() const { return m_version; }

      /**
      * Get the ciphersuite code of the saved session
      */
      uint16_t ciphersuite_code() const { return m_ciphersuite; }

      /**
      * Get the ciphersuite info of the saved session
      */
      Ciphersuite ciphersuite() const { return Ciphersuite::by_id(m_ciphersuite); }

      /**
      * Get which side of the connection the resumed session we are/were
      * acting as.
      */
      Connection_Side side() const { return m_connection_side; }

      /**
      * Get the SRP identity (if sent by the client in the initial handshake)
      */
      const std::string& srp_identifier() const { return m_srp_identifier; }

      /**
      * Get the saved master secret
      */
      const secure_vector<uint8_t>& master_secret() const { return m_master_secret; }

      /**
      * Get the session identifier
      */
      const std::vector<uint8_t>& session_id() const { return m_identifier; }

      /**
      * Get the negotiated DTLS-SRTP algorithm (RFC 5764)
      */
      uint16_t dtls_srtp_profile() const { return m_srtp_profile; }

      bool supports_extended_master_secret() const { return m_extended_master_secret; }

      bool supports_encrypt_then_mac() const { return m_encrypt_then_mac; }

      /**
      * Return the certificate chain of the peer (possibly empty)
      */
      const std::vector<X509_Certificate>& peer_certs() const { return m_peer_certs; }

      /**
      * Get the wall clock time this session began
      */
      std::chrono::system_clock::time_point start_time() const { return m_start_time; }

      /**
      * Return how long this session has existed (in seconds)
      */
      std::chrono::seconds session_age() const;

      /**
      * Return the session ticket the server gave us
      */
      const std::vector<uint8_t>& session_ticket() const { return m_session_ticket; }

      /**
      * @return information about the TLS server
      */
      const Server_Information& server_info() const { return m_server_info; }

   private:
      enum { TLS_SESSION_PARAM_STRUCT_VERSION = 20160812 };

      std::chrono::system_clock::time_point m_start_time;

      std::vector<uint8_t> m_identifier;
      std::vector<uint8_t> m_session_ticket; // only used by client side
      secure_vector<uint8_t> m_master_secret;

      Protocol_Version m_version;
      uint16_t m_ciphersuite;
      Connection_Side m_connection_side;
      uint16_t m_srtp_profile;
      bool m_extended_master_secret;
      bool m_encrypt_then_mac;

      std::vector<X509_Certificate> m_peer_certs;
      Server_Information m_server_info; // optional
      std::string m_srp_identifier; // optional
   };

}

}

#endif
