/*
* TLS Blocking API
* (C) 2013 Jack Lloyd
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_BLOCKING_CHANNELS_H_
#define BOTAN_TLS_BLOCKING_CHANNELS_H_

#include <botan/tls_client.h>

namespace Botan {

namespace TLS {

/**
* Blocking TLS Client
* Can be used directly, or subclass to get handshake and alert notifications
*/
class BOTAN_PUBLIC_API(2,0) Blocking_Client
   {
   public:
      /*
      * These functions are expected to block until completing entirely, or
      * fail by throwing an exception.
      */
      typedef std::function<size_t (uint8_t[], size_t)> read_fn;
      typedef std::function<void (const uint8_t[], size_t)> write_fn;

      BOTAN_DEPRECATED("Use the regular TLS::Client interface")
      Blocking_Client(read_fn reader,
                      write_fn writer,
                      Session_Manager& session_manager,
                      Credentials_Manager& creds,
                      const Policy& policy,
                      RandomNumberGenerator& rng,
                      const Server_Information& server_info = Server_Information(),
                      const Protocol_Version& offer_version = Protocol_Version::latest_tls_version(),
                      const std::vector<std::string>& next_protos = {});

      /**
      * Completes full handshake then returns
      */
      void do_handshake();

      /**
      * Number of bytes pending read in the plaintext buffer (bytes
      * readable without blocking)
      */
      size_t pending() const { return m_plaintext.size(); }

      /**
      * Blocking read, will return at least 1 byte (eventually) or else 0 if the connection
      * is closed.
      */
      size_t read(uint8_t buf[], size_t buf_len);

      void write(const uint8_t buf[], size_t buf_len) { m_channel.send(buf, buf_len); }

      const TLS::Channel& underlying_channel() const { return m_channel; }
      TLS::Channel& underlying_channel() { return m_channel; }

      void close() { m_channel.close(); }

      bool is_closed() const { return m_channel.is_closed(); }

      std::vector<X509_Certificate> peer_cert_chain() const
         { return m_channel.peer_cert_chain(); }

      virtual ~Blocking_Client() = default;

   protected:
      /**
      * Application can override to get the handshake complete notification
      */
      virtual bool handshake_complete(const Session&) { return true; }

      /**
      * Application can override to get notification of alerts
      */
      virtual void alert_notification(const Alert&) {}

   private:

      bool handshake_cb(const Session&);

      void data_cb(const uint8_t data[], size_t data_len);

      void alert_cb(const Alert& alert);

      read_fn m_read;
      std::unique_ptr<Compat_Callbacks> m_callbacks;
      TLS::Client m_channel;
      secure_vector<uint8_t> m_plaintext;
   };

}

}

#endif
