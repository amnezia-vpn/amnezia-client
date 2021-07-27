/*
* PKCS #8
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PKCS8_H_
#define BOTAN_PKCS8_H_

#include <botan/pk_keys.h>
#include <botan/exceptn.h>
#include <botan/secmem.h>
#include <functional>
#include <chrono>
#include <memory>

namespace Botan {

class DataSource;
class RandomNumberGenerator;

/**
* PKCS #8 General Exception
*/
class BOTAN_PUBLIC_API(2,0) PKCS8_Exception final : public Decoding_Error
   {
   public:
      explicit PKCS8_Exception(const std::string& error) :
         Decoding_Error("PKCS #8: " + error) {}
   };

/**
* This namespace contains functions for handling PKCS #8 private keys
*/
namespace PKCS8 {

/**
* BER encode a private key
* @param key the private key to encode
* @return BER encoded key
*/
BOTAN_PUBLIC_API(2,0) secure_vector<uint8_t> BER_encode(const Private_Key& key);

/**
* Get a string containing a PEM encoded private key.
* @param key the key to encode
* @return encoded key
*/
BOTAN_PUBLIC_API(2,0) std::string PEM_encode(const Private_Key& key);

/**
* Encrypt a key using PKCS #8 encryption
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param msec number of milliseconds to run the password derivation
* @param pbe_algo the name of the desired password-based encryption
*        algorithm; if empty ("") a reasonable (portable/secure)
*        default will be chosen.
* @return encrypted key in binary BER form
*/
BOTAN_PUBLIC_API(2,0) std::vector<uint8_t>
BER_encode(const Private_Key& key,
           RandomNumberGenerator& rng,
           const std::string& pass,
           std::chrono::milliseconds msec = std::chrono::milliseconds(300),
           const std::string& pbe_algo = "");

/**
* Get a string containing a PEM encoded private key, encrypting it with a
* password.
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param msec number of milliseconds to run the password derivation
* @param pbe_algo the name of the desired password-based encryption
*        algorithm; if empty ("") a reasonable (portable/secure)
*        default will be chosen.
* @return encrypted key in PEM form
*/
BOTAN_PUBLIC_API(2,0) std::string
PEM_encode(const Private_Key& key,
           RandomNumberGenerator& rng,
           const std::string& pass,
           std::chrono::milliseconds msec = std::chrono::milliseconds(300),
           const std::string& pbe_algo = "");

/**
* Encrypt a key using PKCS #8 encryption and a fixed iteration count
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbkdf_iter number of interations to run PBKDF2
* @param cipher if non-empty specifies the cipher to use. CBC and GCM modes
*   are supported, for example "AES-128/CBC", "AES-256/GCM", "Serpent/CBC".
*   If empty a suitable default is chosen.
* @param pbkdf_hash if non-empty specifies the PBKDF hash function to use.
*   For example "SHA-256" or "SHA-384". If empty a suitable default is chosen.
* @return encrypted key in binary BER form
*/
BOTAN_PUBLIC_API(2,1) std::vector<uint8_t>
BER_encode_encrypted_pbkdf_iter(const Private_Key& key,
                                RandomNumberGenerator& rng,
                                const std::string& pass,
                                size_t pbkdf_iter,
                                const std::string& cipher = "",
                                const std::string& pbkdf_hash = "");

/**
* Get a string containing a PEM encoded private key, encrypting it with a
* password.
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbkdf_iter number of iterations to run PBKDF
* @param cipher if non-empty specifies the cipher to use. CBC and GCM modes
*   are supported, for example "AES-128/CBC", "AES-256/GCM", "Serpent/CBC".
*   If empty a suitable default is chosen.
* @param pbkdf_hash if non-empty specifies the PBKDF hash function to use.
*   For example "SHA-256" or "SHA-384". If empty a suitable default is chosen.
* @return encrypted key in PEM form
*/
BOTAN_PUBLIC_API(2,1) std::string
PEM_encode_encrypted_pbkdf_iter(const Private_Key& key,
                                RandomNumberGenerator& rng,
                                const std::string& pass,
                                size_t pbkdf_iter,
                                const std::string& cipher = "",
                                const std::string& pbkdf_hash = "");

/**
* Encrypt a key using PKCS #8 encryption and a variable iteration count
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbkdf_msec how long to run PBKDF2
* @param pbkdf_iterations if non-null, set to the number of iterations used
* @param cipher if non-empty specifies the cipher to use. CBC and GCM modes
*   are supported, for example "AES-128/CBC", "AES-256/GCM", "Serpent/CBC".
*   If empty a suitable default is chosen.
* @param pbkdf_hash if non-empty specifies the PBKDF hash function to use.
*   For example "SHA-256" or "SHA-384". If empty a suitable default is chosen.
* @return encrypted key in binary BER form
*/
BOTAN_PUBLIC_API(2,1) std::vector<uint8_t>
BER_encode_encrypted_pbkdf_msec(const Private_Key& key,
                                RandomNumberGenerator& rng,
                                const std::string& pass,
                                std::chrono::milliseconds pbkdf_msec,
                                size_t* pbkdf_iterations,
                                const std::string& cipher = "",
                                const std::string& pbkdf_hash = "");

/**
* Get a string containing a PEM encoded private key, encrypting it with a
* password.
* @param key the key to encode
* @param rng the rng to use
* @param pass the password to use for encryption
* @param pbkdf_msec how long in milliseconds to run PBKDF2
* @param pbkdf_iterations (output argument) number of iterations of PBKDF
*  that ended up being used
* @param cipher if non-empty specifies the cipher to use. CBC and GCM modes
*   are supported, for example "AES-128/CBC", "AES-256/GCM", "Serpent/CBC".
*   If empty a suitable default is chosen.
* @param pbkdf_hash if non-empty specifies the PBKDF hash function to use.
*   For example "SHA-256" or "SHA-384". If empty a suitable default is chosen.
* @return encrypted key in PEM form
*/
BOTAN_PUBLIC_API(2,1) std::string
PEM_encode_encrypted_pbkdf_msec(const Private_Key& key,
                                RandomNumberGenerator& rng,
                                const std::string& pass,
                                std::chrono::milliseconds pbkdf_msec,
                                size_t* pbkdf_iterations,
                                const std::string& cipher = "",
                                const std::string& pbkdf_hash = "");

/**
* Load an encrypted key from a data source.
* @param source the data source providing the encoded key
* @param rng ignored for compatibility
* @param get_passphrase a function that returns passphrases
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(DataSource& source,
                                            RandomNumberGenerator& rng,
                                            std::function<std::string ()> get_passphrase);

/** Load an encrypted key from a data source.
* @param source the data source providing the encoded key
* @param rng ignored for compatibility
* @param pass the passphrase to decrypt the key
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(DataSource& source,
                                            RandomNumberGenerator& rng,
                                            const std::string& pass);

/** Load an unencrypted key from a data source.
* @param source the data source providing the encoded key
* @param rng ignored for compatibility
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(DataSource& source,
                                            RandomNumberGenerator& rng);

#if defined(BOTAN_TARGET_OS_HAS_FILESYSTEM)
/**
* Load an encrypted key from a file.
* @param filename the path to the file containing the encoded key
* @param rng ignored for compatibility
* @param get_passphrase a function that returns passphrases
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(const std::string& filename,
                                            RandomNumberGenerator& rng,
                                            std::function<std::string ()> get_passphrase);

/** Load an encrypted key from a file.
* @param filename the path to the file containing the encoded key
* @param rng ignored for compatibility
* @param pass the passphrase to decrypt the key
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(const std::string& filename,
                                            RandomNumberGenerator& rng,
                                            const std::string& pass);

/** Load an unencrypted key from a file.
* @param filename the path to the file containing the encoded key
* @param rng ignored for compatibility
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,0) Private_Key* load_key(const std::string& filename,
                                            RandomNumberGenerator& rng);
#endif

/**
* Copy an existing encoded key object.
* @param key the key to copy
* @param rng ignored for compatibility
* @return new copy of the key
*/
BOTAN_PUBLIC_API(2,0) Private_Key* copy_key(const Private_Key& key,
                                            RandomNumberGenerator& rng);


/**
* Load an encrypted key from a data source.
* @param source the data source providing the encoded key
* @param get_passphrase a function that returns passphrases
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,3)
std::unique_ptr<Private_Key> load_key(DataSource& source,
                                      std::function<std::string ()> get_passphrase);

/** Load an encrypted key from a data source.
* @param source the data source providing the encoded key
* @param pass the passphrase to decrypt the key
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,3)
std::unique_ptr<Private_Key> load_key(DataSource& source,
                                      const std::string& pass);

/** Load an unencrypted key from a data source.
* @param source the data source providing the encoded key
* @return loaded private key object
*/
BOTAN_PUBLIC_API(2,3)
std::unique_ptr<Private_Key> load_key(DataSource& source);

/**
* Copy an existing encoded key object.
* @param key the key to copy
* @return new copy of the key
*/
BOTAN_PUBLIC_API(2,3)
std::unique_ptr<Private_Key> copy_key(const Private_Key& key);

}

}

#endif
