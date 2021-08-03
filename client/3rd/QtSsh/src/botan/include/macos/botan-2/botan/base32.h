/*
* Base32 Encoding and Decoding
* (C) 2018 Erwan Chaussy
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BASE32_CODEC_H_
#define BOTAN_BASE32_CODEC_H_

#include <botan/secmem.h>
#include <string>

namespace Botan {

/**
* Perform base32 encoding
* @param output an array of at least base32_encode_max_output bytes
* @param input is some binary data
* @param input_length length of input in bytes
* @param input_consumed is an output parameter which says how many
*        bytes of input were actually consumed. If less than
*        input_length, then the range input[consumed:length]
*        should be passed in later along with more input.
* @param final_inputs true iff this is the last input, in which case
         padding chars will be applied if needed
* @return number of bytes written to output
*/
size_t BOTAN_PUBLIC_API(2, 7) base32_encode(char output[],
      const uint8_t input[],
      size_t input_length,
      size_t& input_consumed,
      bool final_inputs);

/**
* Perform base32 encoding
* @param input some input
* @param input_length length of input in bytes
* @return base32 representation of input
*/
std::string BOTAN_PUBLIC_API(2, 7) base32_encode(const uint8_t input[],
      size_t input_length);

/**
* Perform base32 encoding
* @param input some input
* @return base32 representation of input
*/
template <typename Alloc>
std::string base32_encode(const std::vector<uint8_t, Alloc>& input)
   {
   return base32_encode(input.data(), input.size());
   }

/**
* Perform base32 decoding
* @param output an array of at least base32_decode_max_output bytes
* @param input some base32 input
* @param input_length length of input in bytes
* @param input_consumed is an output parameter which says how many
*        bytes of input were actually consumed. If less than
*        input_length, then the range input[consumed:length]
*        should be passed in later along with more input.
* @param final_inputs true iff this is the last input, in which case
         padding is allowed
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_PUBLIC_API(2, 7) base32_decode(uint8_t output[],
      const char input[],
      size_t input_length,
      size_t& input_consumed,
      bool final_inputs,
      bool ignore_ws = true);

/**
* Perform base32 decoding
* @param output an array of at least base32_decode_max_output bytes
* @param input some base32 input
* @param input_length length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_PUBLIC_API(2, 7) base32_decode(uint8_t output[],
      const char input[],
      size_t input_length,
      bool ignore_ws = true);

/**
* Perform base32 decoding
* @param output an array of at least base32_decode_max_output bytes
* @param input some base32 input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return number of bytes written to output
*/
size_t BOTAN_PUBLIC_API(2, 7) base32_decode(uint8_t output[],
      const std::string& input,
      bool ignore_ws = true);

/**
* Perform base32 decoding
* @param input some base32 input
* @param input_length the length of input in bytes
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded base32 output
*/
secure_vector<uint8_t> BOTAN_PUBLIC_API(2, 7) base32_decode(const char input[],
      size_t input_length,
      bool ignore_ws = true);

/**
* Perform base32 decoding
* @param input some base32 input
* @param ignore_ws ignore whitespace on input; if false, throw an
                   exception if whitespace is encountered
* @return decoded base32 output
*/
secure_vector<uint8_t> BOTAN_PUBLIC_API(2, 7) base32_decode(const std::string& input,
      bool ignore_ws = true);

} // namespace Botan

#endif
