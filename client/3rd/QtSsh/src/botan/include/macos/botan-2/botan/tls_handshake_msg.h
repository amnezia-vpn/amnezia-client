/*
* TLS Handshake Message
* (C) 2012 Jack Lloyd
*     2016 Matthias Gierlings
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_HANDSHAKE_MSG_H_
#define BOTAN_TLS_HANDSHAKE_MSG_H_

#include <botan/tls_magic.h>
#include <vector>
#include <string>

namespace Botan {

namespace TLS {

class Handshake_IO;
class Handshake_Hash;

/**
* TLS Handshake Message Base Class
*/
class BOTAN_PUBLIC_API(2,0) Handshake_Message
   {
   public:
      /**
      * @return string representation of this message type
      */
      std::string type_string() const;

      /**
      * @return the message type
      */
      virtual Handshake_Type type() const = 0;

      /**
      * @return DER representation of this message
      */
      virtual std::vector<uint8_t> serialize() const = 0;

      virtual ~Handshake_Message() = default;
   };

}

}

#endif
