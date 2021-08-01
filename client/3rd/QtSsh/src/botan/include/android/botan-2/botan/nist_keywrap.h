/*
* (C) 2011,2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_NIST_KEY_WRAP_H_
#define BOTAN_NIST_KEY_WRAP_H_

#include <botan/secmem.h>

namespace Botan {

class BlockCipher;

/**
* Key wrap. See RFC 3394 and NIST SP800-38F
* @param input the value to be encrypted
* @param input_len length of input, must be a multiple of 8
* @param bc a keyed 128-bit block cipher that will be used to encrypt input
* @return input encrypted under NIST key wrap algorithm
*/
std::vector<uint8_t> BOTAN_PUBLIC_API(2,4)
nist_key_wrap(const uint8_t input[],
              size_t input_len,
              const BlockCipher& bc);

/**
* @param input the value to be decrypted, output of nist_key_wrap
* @param input_len length of input
* @param bc a keyed 128-bit block cipher that will be used to decrypt input
* @return input decrypted under NIST key wrap algorithm
* Throws an exception if decryption fails.
*/
secure_vector<uint8_t> BOTAN_PUBLIC_API(2,4)
nist_key_unwrap(const uint8_t input[],
                size_t input_len,
                const BlockCipher& bc);

/**
* KWP (key wrap with padding). See RFC 5649 and NIST SP800-38F
* @param input the value to be encrypted
* @param input_len length of input
* @param bc a keyed 128-bit block cipher that will be used to encrypt input
* @return input encrypted under NIST key wrap algorithm
*/
std::vector<uint8_t> BOTAN_PUBLIC_API(2,4)
nist_key_wrap_padded(const uint8_t input[],
                     size_t input_len,
                     const BlockCipher& bc);

/**
* @param input the value to be decrypted, output of nist_key_wrap
* @param input_len length of input
* @param bc a keyed 128-bit block cipher that will be used to decrypt input
* @return input decrypted under NIST key wrap algorithm
* Throws an exception if decryption fails.
*/
secure_vector<uint8_t> BOTAN_PUBLIC_API(2,4)
nist_key_unwrap_padded(const uint8_t input[],
                       size_t input_len,
                       const BlockCipher& bc);


}

#endif
