/*
* OID Registry
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_OIDS_H_
#define BOTAN_OIDS_H_

#include <botan/asn1_obj.h>
#include <unordered_map>

namespace Botan {

namespace OIDS {

/**
* Register an OID to string mapping.
* @param oid the oid to register
* @param name the name to be associated with the oid
*/
BOTAN_UNSTABLE_API void add_oid(const OID& oid, const std::string& name);

BOTAN_UNSTABLE_API void add_oid2str(const OID& oid, const std::string& name);
BOTAN_UNSTABLE_API void add_str2oid(const OID& oid, const std::string& name);

BOTAN_UNSTABLE_API void add_oidstr(const char* oidstr, const char* name);

std::unordered_map<std::string, std::string> load_oid2str_map();
std::unordered_map<std::string, OID> load_str2oid_map();

/**
* Resolve an OID
* @param oid the OID to look up
* @return name associated with this OID, or an empty string
*/
BOTAN_UNSTABLE_API std::string oid2str_or_empty(const OID& oid);

/**
* Find the OID to a name. The lookup will be performed in the
* general OID section of the configuration.
* @param name the name to resolve
* @return OID associated with the specified name
*/
BOTAN_UNSTABLE_API OID str2oid_or_empty(const std::string& name);

BOTAN_UNSTABLE_API std::string oid2str_or_throw(const OID& oid);

/**
* See if an OID exists in the internal table.
* @param oid the oid to check for
* @return true if the oid is registered
*/
BOTAN_UNSTABLE_API bool BOTAN_DEPRECATED("Just lookup the value instead") have_oid(const std::string& oid);

/**
* Tests whether the specified OID stands for the specified name.
* @param oid the OID to check
* @param name the name to check
* @return true if the specified OID stands for the specified name
*/
inline bool BOTAN_DEPRECATED("Use oid == OID::from_string(name)") name_of(const OID& oid, const std::string& name)
   {
   return (oid == str2oid_or_empty(name));
   }

/**
* Prefer oid2str_or_empty
*/
inline std::string lookup(const OID& oid)
   {
   return oid2str_or_empty(oid);
   }

/**
* Prefer str2oid_or_empty
*/
inline OID lookup(const std::string& name)
   {
   return str2oid_or_empty(name);
   }

inline std::string BOTAN_DEPRECATED("Use oid2str_or_empty") oid2str(const OID& oid)
   {
   return oid2str_or_empty(oid);
   }

inline OID BOTAN_DEPRECATED("Use str2oid_or_empty") str2oid(const std::string& name)
   {
   return str2oid_or_empty(name);
   }

}

}

#endif
