/*
* (C) 2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BASE58_CODEC_H_
#define BOTAN_BASE58_CODEC_H_

#include <botan/secmem.h>
#include <vector>
#include <string>

namespace Botan {

/**
* Perform base58 encoding
*
* This is raw base58 encoding, without the checksum
*/
std::string
BOTAN_PUBLIC_API(2,9) base58_encode(const uint8_t input[],
                                    size_t input_length);

/**
* Perform base58 encoding with checksum
*/
std::string
BOTAN_PUBLIC_API(2,9) base58_check_encode(const uint8_t input[],
                                          size_t input_length);


/**
* Perform base58 decoding
*
* This is raw base58 encoding, without the checksum
*/
std::vector<uint8_t>
BOTAN_PUBLIC_API(2,9) base58_decode(const char input[],
                                    size_t input_length);

/**
* Perform base58 decoding with checksum
*/
std::vector<uint8_t>
BOTAN_PUBLIC_API(2,9) base58_check_decode(const char input[],
                                          size_t input_length);


// Some convenience wrappers:

template<typename Alloc>
inline std::string base58_encode(const std::vector<uint8_t, Alloc>& vec)
   {
   return base58_encode(vec.data(), vec.size());
   }

template<typename Alloc>
inline std::string base58_check_encode(const std::vector<uint8_t, Alloc>& vec)
   {
   return base58_check_encode(vec.data(), vec.size());
   }

inline std::vector<uint8_t> base58_decode(const std::string& s)
   {
   return base58_decode(s.data(), s.size());
   }

inline std::vector<uint8_t> base58_check_decode(const std::string& s)
   {
   return base58_check_decode(s.data(), s.size());
   }

}

#endif
