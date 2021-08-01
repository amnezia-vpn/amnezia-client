/*
* Rivest's Package Tranform
* (C) 2009 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_AONT_PACKAGE_TRANSFORM_H_
#define BOTAN_AONT_PACKAGE_TRANSFORM_H_

#include <botan/block_cipher.h>

namespace Botan {

class RandomNumberGenerator;

/**
* Rivest's Package Tranform
* @param rng the random number generator to use
* @param cipher the block cipher to use (aont_package takes ownership)
* @param input the input data buffer
* @param input_len the length of the input data in bytes
* @param output the output data buffer (must be at least
*        input_len + cipher->BLOCK_SIZE bytes long)
*/
BOTAN_DEPRECATED("Possibly broken, avoid")
void BOTAN_PUBLIC_API(2,0)
aont_package(RandomNumberGenerator& rng,
             BlockCipher* cipher,
             const uint8_t input[], size_t input_len,
             uint8_t output[]);

/**
* Rivest's Package Tranform (Inversion)
* @param cipher the block cipher to use (aont_package takes ownership)
* @param input the input data buffer
* @param input_len the length of the input data in bytes
* @param output the output data buffer (must be at least
*        input_len - cipher->BLOCK_SIZE bytes long)
*/
BOTAN_DEPRECATED("Possibly broken, avoid")
void BOTAN_PUBLIC_API(2,0)
aont_unpackage(BlockCipher* cipher,
               const uint8_t input[], size_t input_len,
               uint8_t output[]);

}

#endif
