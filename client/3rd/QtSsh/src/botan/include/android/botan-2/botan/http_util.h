/*
* HTTP utilities
* (C) 2013 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_UTILS_URLGET_H_
#define BOTAN_UTILS_URLGET_H_

#include <botan/types.h>
#include <botan/exceptn.h>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <chrono>

BOTAN_FUTURE_INTERNAL_HEADER(http_util.h)

namespace Botan {

namespace HTTP {

/**
* HTTP_Error Exception
*/
class BOTAN_PUBLIC_API(2,0) HTTP_Error final : public Exception
   {
   public:
      explicit HTTP_Error(const std::string& msg) :
         Exception("HTTP error " + msg)
         {}

      ErrorType error_type() const noexcept override { return ErrorType::HttpError; }

   };

class Response final
   {
   public:
      Response() : m_status_code(0), m_status_message("Uninitialized") {}

      Response(unsigned int status_code, const std::string& status_message,
               const std::vector<uint8_t>& body,
               const std::map<std::string, std::string>& headers) :
         m_status_code(status_code),
         m_status_message(status_message),
         m_body(body),
         m_headers(headers) {}

      unsigned int status_code() const { return m_status_code; }

      const std::vector<uint8_t>& body() const { return m_body; }

      const std::map<std::string, std::string>& headers() const { return m_headers; }

      std::string status_message() const { return m_status_message; }

      void throw_unless_ok()
         {
         if(status_code() != 200)
            throw HTTP_Error(status_message());
         }

   private:
      unsigned int m_status_code;
      std::string m_status_message;
      std::vector<uint8_t> m_body;
      std::map<std::string, std::string> m_headers;
   };

BOTAN_PUBLIC_API(2,0) std::ostream& operator<<(std::ostream& o, const Response& resp);

typedef std::function<std::string (const std::string&, const std::string&, const std::string&)> http_exch_fn;

BOTAN_PUBLIC_API(2,0) Response http_sync(http_exch_fn fn,
                                         const std::string& verb,
                                         const std::string& url,
                                         const std::string& content_type,
                                         const std::vector<uint8_t>& body,
                                         size_t allowable_redirects);

BOTAN_PUBLIC_API(2,0) Response http_sync(const std::string& verb,
                                         const std::string& url,
                                         const std::string& content_type,
                                         const std::vector<uint8_t>& body,
                                         size_t allowable_redirects,
                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

BOTAN_PUBLIC_API(2,0) Response GET_sync(const std::string& url,
                                        size_t allowable_redirects = 1,
                                        std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

BOTAN_PUBLIC_API(2,0) Response POST_sync(const std::string& url,
                                         const std::string& content_type,
                                         const std::vector<uint8_t>& body,
                                         size_t allowable_redirects = 1,
                                         std::chrono::milliseconds timeout = std::chrono::milliseconds(3000));

BOTAN_PUBLIC_API(2,0) std::string url_encode(const std::string& url);

}

}

#endif
