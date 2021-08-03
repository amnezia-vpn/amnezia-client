/*
* Exceptions
* (C) 2004-2006 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_EXCEPTION_H_
#define BOTAN_TLS_EXCEPTION_H_

#include <botan/exceptn.h>
#include <botan/tls_alert.h>

namespace Botan {

namespace TLS {

/**
* TLS Exception Base Class
*/
class BOTAN_PUBLIC_API(2,0) TLS_Exception : public Exception
   {
   public:
      Alert::Type type() const { return m_alert_type; }

      TLS_Exception(Alert::Type type,
                    const std::string& err_msg = "Unknown error") :
         Exception(err_msg), m_alert_type(type) {}

      int error_code() const noexcept override { return static_cast<int>(m_alert_type); }

      ErrorType error_type() const noexcept override { return ErrorType::TLSError; }

   private:
      Alert::Type m_alert_type;
   };

/**
* Unexpected_Message Exception
*/
class BOTAN_PUBLIC_API(2,0) Unexpected_Message final : public TLS_Exception
   {
   public:
      explicit Unexpected_Message(const std::string& err) :
         TLS_Exception(Alert::UNEXPECTED_MESSAGE, err) {}
   };

}

}

#endif
