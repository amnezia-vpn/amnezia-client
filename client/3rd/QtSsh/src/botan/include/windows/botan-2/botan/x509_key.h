/*
* X.509 Public Key
* (C) 1999-2010 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_X509_PUBLIC_KEY_H_
#define BOTAN_X509_PUBLIC_KEY_H_

#include <botan/pk_keys.h>
#include <botan/types.h>
#include <string>
#include <vector>

namespace Botan {

class RandomNumberGenerator;
class DataSource;

/**
* The two types of X509 encoding supported by Botan.
* This enum is not used anymore, and will be removed in a future major release.
*/
enum X509_Encoding { RAW_BER, PEM };

/**
* This namespace contains functions for handling X.509 public keys
*/
namespace X509 {

/**
* BER encode a key
* @param key the public key to encode
* @return BER encoding of this key
*/
BOTAN_PUBLIC_API(2,0) std::vector<uint8_t> BER_encode(const Public_Key& key);

/**
* PEM encode a public key into a string.
* @param key the key to encode
* @return PEM encoded key
*/
BOTAN_PUBLIC_API(2,0) std::string PEM_encode(const Public_Key& key);

/**
* Create a public key from a data source.
* @param source the source providing the DER or PEM encoded key
* @return new public key object
*/
BOTAN_PUBLIC_API(2,0) Public_Key* load_key(DataSource& source);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
/**
* Create a public key from a file
* @param filename pathname to the file to load
* @return new public key object
*/
BOTAN_PUBLIC_API(2,0) Public_Key* load_key(const std::string& filename);
#endif

/**
* Create a public key from a memory region.
* @param enc the memory region containing the DER or PEM encoded key
* @return new public key object
*/
BOTAN_PUBLIC_API(2,0) Public_Key* load_key(const std::vector<uint8_t>& enc);

/**
* Copy a key.
* @param key the public key to copy
* @return new public key object
*/
BOTAN_PUBLIC_API(2,0) Public_Key* copy_key(const Public_Key& key);

}

}

#endif
