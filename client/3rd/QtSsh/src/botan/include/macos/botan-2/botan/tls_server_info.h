/*
* TLS Server Information
* (C) 2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_SERVER_INFO_H_
#define BOTAN_TLS_SERVER_INFO_H_

#include <botan/types.h>
#include <string>

namespace Botan {

namespace TLS {

/**
* Represents information known about a TLS server.
*/
class BOTAN_PUBLIC_API(2,0) Server_Information final
   {
   public:
      /**
      * An empty server info - nothing known
      */
      Server_Information() : m_hostname(""), m_service(""), m_port(0) {}

      /**
      * @param hostname the host's DNS name, if known
      * @param port specifies the protocol port of the server (eg for
      *        TCP/UDP). Zero represents unknown.
      */
      Server_Information(const std::string& hostname,
                        uint16_t port = 0) :
         m_hostname(hostname), m_service(""), m_port(port) {}

      /**
      * @param hostname the host's DNS name, if known
      * @param service is a text string of the service type
      *        (eg "https", "tor", or "git")
      * @param port specifies the protocol port of the server (eg for
      *        TCP/UDP). Zero represents unknown.
      */
      Server_Information(const std::string& hostname,
                        const std::string& service,
                        uint16_t port = 0) :
         m_hostname(hostname), m_service(service), m_port(port) {}

      /**
      * @return the host's DNS name, if known
      */
      std::string hostname() const { return m_hostname; }

      /**
      * @return text string of the service type, e.g.,
      * "https", "tor", or "git"
      */
      std::string service() const { return m_service; }

      /**
      * @return the protocol port of the server, or zero if unknown
      */
      uint16_t port() const { return m_port; }

      /**
      * @return whether the hostname is known
      */
      bool empty() const { return m_hostname.empty(); }

   private:
      std::string m_hostname, m_service;
      uint16_t m_port;
   };

inline bool operator==(const Server_Information& a, const Server_Information& b)
   {
   return (a.hostname() == b.hostname()) &&
          (a.service() == b.service()) &&
          (a.port() == b.port());

   }

inline bool operator!=(const Server_Information& a, const Server_Information& b)
   {
   return !(a == b);
   }

inline bool operator<(const Server_Information& a, const Server_Information& b)
   {
   if(a.hostname() != b.hostname())
      return (a.hostname() < b.hostname());
   if(a.service() != b.service())
      return (a.service() < b.service());
   if(a.port() != b.port())
      return (a.port() < b.port());
   return false; // equal
   }

}

}

#endif
