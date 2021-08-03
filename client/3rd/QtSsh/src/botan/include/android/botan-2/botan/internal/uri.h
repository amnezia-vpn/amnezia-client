/*
* (C) 2019 Nuno Goncalves <nunojpg@gmail.com>
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_URI_H_
#define BOTAN_URI_H_

#include <cstdint>
#include <string>

#include <botan/build.h>

namespace Botan {

struct BOTAN_TEST_API URI
   {
   enum class Type : uint8_t
      {
      NotSet,
      IPv4,
      IPv6,
      Domain,
      };
   static URI fromAny(const std::string& uri);
   static URI fromIPv4(const std::string& uri);
   static URI fromIPv6(const std::string& uri);
   static URI fromDomain(const std::string& uri);
   URI() = default;
   URI(Type xtype, const std::string& xhost, unsigned short xport)
      : type { xtype }
      , host { xhost }
      , port { xport }
      {}
   bool operator==(const URI& a) const
      {
      return type == a.type && host == a.host && port == a.port;
      }
   std::string to_string() const;

   const Type type{Type::NotSet};
   const std::string host{};
   const uint16_t port{};
   };

}

#endif
