/*
* Algorithm Lookup
* (C) 1999-2007,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_LOOKUP_H_
#define BOTAN_LOOKUP_H_

#include <botan/build.h>
#include <botan/exceptn.h>
#include <string>
#include <vector>
#include <memory>

#if defined(BOTAN_HAS_BLOCK_CIPHER)
   #include <botan/block_cipher.h>
#endif

#if defined(BOTAN_HAS_STREAM_CIPHER)
   #include <botan/stream_cipher.h>
#endif

#if defined(BOTAN_HAS_HASH)
   #include <botan/hash.h>
#endif

#if defined(BOTAN_HAS_MAC)
   #include <botan/mac.h>
#endif

namespace Botan {

BOTAN_DEPRECATED_HEADER(lookup.h)

/*
* As of 1.11.26 this header is deprecated. Instead use the calls T::create and
* T::providers (as demonstrated in the implementation below).
*/

/*
* Get an algorithm object
*  NOTE: these functions create and return new objects, letting the
* caller assume ownership of them
*/

#if defined(BOTAN_HAS_BLOCK_CIPHER)

/**
* Block cipher factory method.
*
* @param algo_spec the name of the desired block cipher
* @param provider the provider to use
* @return pointer to the block cipher object
*/
BOTAN_DEPRECATED("Use BlockCipher::create")
inline BlockCipher* get_block_cipher(const std::string& algo_spec,
                                     const std::string& provider = "")
   {
   return BlockCipher::create(algo_spec, provider).release();
   }

BOTAN_DEPRECATED("Use BlockCipher::create_or_throw")
inline std::unique_ptr<BlockCipher> make_block_cipher(const std::string& algo_spec,
                                                      const std::string& provider = "")
   {
   return BlockCipher::create_or_throw(algo_spec, provider);
   }

BOTAN_DEPRECATED("Use BlockCipher::providers")
inline std::vector<std::string> get_block_cipher_providers(const std::string& algo_spec)
   {
   return BlockCipher::providers(algo_spec);
   }

#endif

#if defined(BOTAN_HAS_STREAM_CIPHER)

/**
* Stream cipher factory method.
*
* @param algo_spec the name of the desired stream cipher
* @param provider the provider to use
* @return pointer to the stream cipher object
*/
BOTAN_DEPRECATED("Use StreamCipher::create")
inline StreamCipher* get_stream_cipher(const std::string& algo_spec,
                                       const std::string& provider = "")
   {
   return StreamCipher::create(algo_spec, provider).release();
   }

BOTAN_DEPRECATED("Use StreamCipher::create_or_throw")
inline std::unique_ptr<StreamCipher> make_stream_cipher(const std::string& algo_spec,
                                                        const std::string& provider = "")
   {
   return StreamCipher::create_or_throw(algo_spec, provider);
   }

BOTAN_DEPRECATED("Use StreamCipher::providers")
inline std::vector<std::string> get_stream_cipher_providers(const std::string& algo_spec)
   {
   return StreamCipher::providers(algo_spec);
   }

#endif

#if defined(BOTAN_HAS_HASH)

/**
* Hash function factory method.
*
* @param algo_spec the name of the desired hash function
* @param provider the provider to use
* @return pointer to the hash function object
*/
BOTAN_DEPRECATED("Use HashFunction::create")
inline HashFunction* get_hash_function(const std::string& algo_spec,
                                       const std::string& provider = "")
   {
   return HashFunction::create(algo_spec, provider).release();
   }

BOTAN_DEPRECATED("Use HashFunction::create_or_throw")
inline std::unique_ptr<HashFunction> make_hash_function(const std::string& algo_spec,
                                                        const std::string& provider = "")
   {
   return HashFunction::create_or_throw(algo_spec, provider);
   }

BOTAN_DEPRECATED("Use HashFunction::create")
inline HashFunction* get_hash(const std::string& algo_spec,
                              const std::string& provider = "")
   {
   return HashFunction::create(algo_spec, provider).release();
   }

BOTAN_DEPRECATED("Use HashFunction::providers")
inline std::vector<std::string> get_hash_function_providers(const std::string& algo_spec)
   {
   return HashFunction::providers(algo_spec);
   }

#endif

#if defined(BOTAN_HAS_MAC)
/**
* MAC factory method.
*
* @param algo_spec the name of the desired MAC
* @param provider the provider to use
* @return pointer to the MAC object
*/
BOTAN_DEPRECATED("MessageAuthenticationCode::create")
inline MessageAuthenticationCode* get_mac(const std::string& algo_spec,
                                             const std::string& provider = "")
   {
   return MessageAuthenticationCode::create(algo_spec, provider).release();
   }

BOTAN_DEPRECATED("MessageAuthenticationCode::create_or_throw")
inline std::unique_ptr<MessageAuthenticationCode> make_message_auth(const std::string& algo_spec,
                                                                       const std::string& provider = "")
   {
   return MessageAuthenticationCode::create(algo_spec, provider);
   }

BOTAN_DEPRECATED("MessageAuthenticationCode::providers")
inline std::vector<std::string> get_mac_providers(const std::string& algo_spec)
   {
   return MessageAuthenticationCode::providers(algo_spec);
   }
#endif

}

#endif
