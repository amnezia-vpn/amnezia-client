/*
* Various string utils and parsing functions
* (C) 1999-2007,2013 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PARSING_UTILS_H_
#define BOTAN_PARSING_UTILS_H_

#include <botan/types.h>
#include <string>
#include <vector>
#include <set>

#include <istream>
#include <functional>
#include <map>

BOTAN_FUTURE_INTERNAL_HEADER(parsing.h)

namespace Botan {

/**
* Parse a SCAN-style algorithm name
* @param scan_name the name
* @return the name components
*/
BOTAN_PUBLIC_API(2,0) std::vector<std::string>
parse_algorithm_name(const std::string& scan_name);

/**
* Split a string
* @param str the input string
* @param delim the delimitor
* @return string split by delim
*/
BOTAN_PUBLIC_API(2,0) std::vector<std::string> split_on(
   const std::string& str, char delim);

/**
* Split a string on a character predicate
* @param str the input string
* @param pred the predicate
*
* This function will likely be removed in a future release
*/
BOTAN_PUBLIC_API(2,0) std::vector<std::string>
split_on_pred(const std::string& str,
              std::function<bool (char)> pred);

/**
* Erase characters from a string
*/
BOTAN_PUBLIC_API(2,0)
BOTAN_DEPRECATED("Unused")
std::string erase_chars(const std::string& str, const std::set<char>& chars);

/**
* Replace a character in a string
* @param str the input string
* @param from_char the character to replace
* @param to_char the character to replace it with
* @return str with all instances of from_char replaced by to_char
*/
BOTAN_PUBLIC_API(2,0)
BOTAN_DEPRECATED("Unused")
std::string replace_char(const std::string& str,
                         char from_char,
                         char to_char);

/**
* Replace a character in a string
* @param str the input string
* @param from_chars the characters to replace
* @param to_char the character to replace it with
* @return str with all instances of from_chars replaced by to_char
*/
BOTAN_PUBLIC_API(2,0)
BOTAN_DEPRECATED("Unused")
std::string replace_chars(const std::string& str,
                          const std::set<char>& from_chars,
                          char to_char);

/**
* Join a string
* @param strs strings to join
* @param delim the delimitor
* @return string joined by delim
*/
BOTAN_PUBLIC_API(2,0)
std::string string_join(const std::vector<std::string>& strs,
                        char delim);

/**
* Parse an ASN.1 OID
* @param oid the OID in string form
* @return OID components
*/
BOTAN_PUBLIC_API(2,0) std::vector<uint32_t>
BOTAN_DEPRECATED("Use OID::from_string(oid).get_components()") parse_asn1_oid(const std::string& oid);

/**
* Compare two names using the X.509 comparison algorithm
* @param name1 the first name
* @param name2 the second name
* @return true if name1 is the same as name2 by the X.509 comparison rules
*/
BOTAN_PUBLIC_API(2,0)
bool x500_name_cmp(const std::string& name1,
                   const std::string& name2);

/**
* Convert a string to a number
* @param str the string to convert
* @return number value of the string
*/
BOTAN_PUBLIC_API(2,0) uint32_t to_u32bit(const std::string& str);

/**
* Convert a string to a number
* @param str the string to convert
* @return number value of the string
*/
BOTAN_PUBLIC_API(2,3) uint16_t to_uint16(const std::string& str);

/**
* Convert a time specification to a number
* @param timespec the time specification
* @return number of seconds represented by timespec
*/
BOTAN_PUBLIC_API(2,0) uint32_t BOTAN_DEPRECATED("Not used anymore")
timespec_to_u32bit(const std::string& timespec);

/**
* Convert a string representation of an IPv4 address to a number
* @param ip_str the string representation
* @return integer IPv4 address
*/
BOTAN_PUBLIC_API(2,0) uint32_t string_to_ipv4(const std::string& ip_str);

/**
* Convert an IPv4 address to a string
* @param ip_addr the IPv4 address to convert
* @return string representation of the IPv4 address
*/
BOTAN_PUBLIC_API(2,0) std::string ipv4_to_string(uint32_t ip_addr);

std::map<std::string, std::string> BOTAN_PUBLIC_API(2,0) read_cfg(std::istream& is);

/**
* Accepts key value pairs deliminated by commas:
*
* "" (returns empty map)
* "K=V" (returns map {'K': 'V'})
* "K1=V1,K2=V2"
* "K1=V1,K2=V2,K3=V3"
* "K1=V1,K2=V2,K3=a_value\,with\,commas_and_\=equals"
*
* Values may be empty, keys must be non-empty and unique. Duplicate
* keys cause an exception.
*
* Within both key and value, comma and equals can be escaped with
* backslash. Backslash can also be escaped.
*/
std::map<std::string, std::string> BOTAN_PUBLIC_API(2,8) read_kv(const std::string& kv);

std::string BOTAN_PUBLIC_API(2,0) clean_ws(const std::string& s);

std::string tolower_string(const std::string& s);

/**
* Check if the given hostname is a match for the specified wildcard
*/
bool BOTAN_PUBLIC_API(2,0) host_wildcard_match(const std::string& wildcard,
                                               const std::string& host);


}

#endif
