/*
* Data Store
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_DATA_STORE_H_
#define BOTAN_DATA_STORE_H_

#include <botan/pkix_types.h>
#include <functional>
#include <string>
#include <vector>
#include <map>

BOTAN_FUTURE_INTERNAL_HEADER(datastor.h)

namespace Botan {

/**
* Data Store
*
* This class is used internally by the library, and exposed for ABI
* reasons. There is no reason for applications to use this type directly.
* It will be removed in a future major release.
*/
class BOTAN_UNSTABLE_API Data_Store final
   {
   public:
      /**
      * A search function
      */
      bool operator==(const Data_Store&) const;

      std::multimap<std::string, std::string> search_for(
         std::function<bool (std::string, std::string)> predicate) const;

      std::vector<std::string> get(const std::string&) const;

      std::string get1(const std::string& key) const;

      std::string get1(const std::string& key,
                       const std::string& default_value) const;

      std::vector<uint8_t> get1_memvec(const std::string&) const;
      uint32_t get1_uint32(const std::string&, uint32_t = 0) const;

      bool has_value(const std::string&) const;

      void add(const std::multimap<std::string, std::string>&);
      void add(const std::string&, const std::string&);
      void add(const std::string&, uint32_t);
      void add(const std::string&, const secure_vector<uint8_t>&);
      void add(const std::string&, const std::vector<uint8_t>&);
   private:
      std::multimap<std::string, std::string> m_contents;
   };

/*
* Data Store Extraction Operations
*/

/*
* Create and populate a X509_DN
* @param info data store containing DN information
* @return DN containing attributes from data store
*/
BOTAN_PUBLIC_API(2,0) X509_DN
BOTAN_DEPRECATED("Avoid roundtripping names through Data_Store")
create_dn(const Data_Store& info);

/*
* Create and populate an AlternativeName
* @param info data store containing AlternativeName information
* @return AlternativeName containing attributes from data store
*/
BOTAN_PUBLIC_API(2,0) AlternativeName
BOTAN_DEPRECATED("Avoid roundtripping names through Data_Store")
create_alt_name(const Data_Store& info);


}

#endif
