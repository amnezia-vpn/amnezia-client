/*
* TLS Handshake Hash
* (C) 2004-2006,2011,2012 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_HANDSHAKE_HASH_H_
#define BOTAN_TLS_HANDSHAKE_HASH_H_

#include <botan/secmem.h>
#include <botan/tls_version.h>

namespace Botan {

namespace TLS {

/**
* TLS Handshake Hash
*/
class Handshake_Hash final
   {
   public:
      void update(const uint8_t in[], size_t length)
         { m_data += std::make_pair(in, length); }

      void update(const std::vector<uint8_t>& in)
         { m_data += in; }

      secure_vector<uint8_t> final(Protocol_Version version,
                                const std::string& mac_algo) const;

      const std::vector<uint8_t>& get_contents() const { return m_data; }

      void reset() { m_data.clear(); }
   private:
      std::vector<uint8_t> m_data;
   };

}

}

#endif
