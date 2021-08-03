/*
* TLS Protocol Version Management
* (C) 2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_PROTOCOL_VERSION_H_
#define BOTAN_TLS_PROTOCOL_VERSION_H_

#include <botan/types.h>
#include <string>

namespace Botan {

namespace TLS {

/**
* TLS Protocol Version
*/
class BOTAN_PUBLIC_API(2,0) Protocol_Version final
   {
   public:
      enum Version_Code {
         TLS_V10            = 0x0301,
         TLS_V11            = 0x0302,
         TLS_V12            = 0x0303,

         DTLS_V10           = 0xFEFF,
         DTLS_V12           = 0xFEFD
      };

      /**
      * @return latest known TLS version
      */
      static Protocol_Version latest_tls_version()
         {
         return Protocol_Version(TLS_V12);
         }

      /**
      * @return latest known DTLS version
      */
      static Protocol_Version latest_dtls_version()
         {
         return Protocol_Version(DTLS_V12);
         }

      Protocol_Version() : m_version(0) {}

      explicit Protocol_Version(uint16_t code) : m_version(code) {}

      /**
      * @param named_version a specific named version of the protocol
      */
      Protocol_Version(Version_Code named_version) :
         Protocol_Version(static_cast<uint16_t>(named_version)) {}

      /**
      * @param major the major version
      * @param minor the minor version
      */
      Protocol_Version(uint8_t major, uint8_t minor) :
         Protocol_Version(static_cast<uint16_t>((static_cast<uint16_t>(major) << 8) | minor)) {}

      /**
      * @return true if this is a valid protocol version
      */
      bool valid() const { return (m_version != 0); }

      /**
      * @return true if this is a protocol version we know about
      */
      bool known_version() const;

      /**
      * @return major version of the protocol version
      */
      uint8_t major_version() const { return static_cast<uint8_t>(m_version >> 8); }

      /**
      * @return minor version of the protocol version
      */
      uint8_t minor_version() const { return static_cast<uint8_t>(m_version & 0xFF); }

      /**
      * @return the version code
      */
      uint16_t version_code() const { return m_version; }

      /**
      * @return human-readable description of this version
      */
      std::string to_string() const;

      /**
      * @return true iff this is a DTLS version
      */
      bool is_datagram_protocol() const;

      /**
      * @return true if this version supports negotiable signature algorithms
      */
      bool supports_negotiable_signature_algorithms() const;

      /**
      * @return true if this version uses explicit IVs for block ciphers
      */
      bool supports_explicit_cbc_ivs() const;

      /**
      * @return true if this version uses a ciphersuite specific PRF
      */
      bool supports_ciphersuite_specific_prf() const;

      bool supports_aead_modes() const;

      /**
      * @return if this version is equal to other
      */
      bool operator==(const Protocol_Version& other) const
         {
         return (m_version == other.m_version);
         }

      /**
      * @return if this version is not equal to other
      */
      bool operator!=(const Protocol_Version& other) const
         {
         return (m_version != other.m_version);
         }

      /**
      * @return if this version is later than other
      */
      bool operator>(const Protocol_Version& other) const;

      /**
      * @return if this version is later than or equal to other
      */
      bool operator>=(const Protocol_Version& other) const
         {
         return (*this == other || *this > other);
         }

   private:
      uint16_t m_version;
   };

}

}

#endif

