/*
* (C) 2010,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PK_OPERATIONS_H_
#define BOTAN_PK_OPERATIONS_H_

/**
* Ordinary applications should never need to include or use this
* header. It is exposed only for specialized applications which want
* to implement new versions of public key crypto without merging them
* as changes to the library. One actual example of such usage is an
* application which creates RSA signatures using a custom TPM library.
* Unless you're doing something like that, you don't need anything
* here. Instead use pubkey.h which wraps these types safely and
* provides a stable application-oriented API.
*/

#include <botan/pk_keys.h>
#include <botan/secmem.h>

namespace Botan {

class RandomNumberGenerator;
class EME;
class KDF;
class EMSA;

namespace PK_Ops {

/**
* Public key encryption interface
*/
class BOTAN_PUBLIC_API(2,0) Encryption
   {
   public:
      virtual secure_vector<uint8_t> encrypt(const uint8_t msg[],
                                          size_t msg_len,
                                          RandomNumberGenerator& rng) = 0;

      virtual size_t max_input_bits() const = 0;

      virtual size_t ciphertext_length(size_t ptext_len) const = 0;

      virtual ~Encryption() = default;
   };

/**
* Public key decryption interface
*/
class BOTAN_PUBLIC_API(2,0) Decryption
   {
   public:
      virtual secure_vector<uint8_t> decrypt(uint8_t& valid_mask,
                                          const uint8_t ciphertext[],
                                          size_t ciphertext_len) = 0;

      virtual size_t plaintext_length(size_t ctext_len) const = 0;

      virtual ~Decryption() = default;
   };

/**
* Public key signature verification interface
*/
class BOTAN_PUBLIC_API(2,0) Verification
   {
   public:
      /*
      * Add more data to the message currently being signed
      * @param msg the message
      * @param msg_len the length of msg in bytes
      */
      virtual void update(const uint8_t msg[], size_t msg_len) = 0;

      /*
      * Perform a verification operation
      * @param rng a random number generator
      */
      virtual bool is_valid_signature(const uint8_t sig[], size_t sig_len) = 0;

      virtual ~Verification() = default;
   };

/**
* Public key signature creation interface
*/
class BOTAN_PUBLIC_API(2,0) Signature
   {
   public:
      /*
      * Add more data to the message currently being signed
      * @param msg the message
      * @param msg_len the length of msg in bytes
      */
      virtual void update(const uint8_t msg[], size_t msg_len) = 0;

      /*
      * Perform a signature operation
      * @param rng a random number generator
      */
      virtual secure_vector<uint8_t> sign(RandomNumberGenerator& rng) = 0;

      /*
      * Return an upper bound on the length of the output signature
      */
      virtual size_t signature_length() const = 0;

      virtual ~Signature() = default;
   };

/**
* A generic key agreement operation (eg DH or ECDH)
*/
class BOTAN_PUBLIC_API(2,0) Key_Agreement
   {
   public:
      virtual secure_vector<uint8_t> agree(size_t key_len,
                                           const uint8_t other_key[], size_t other_key_len,
                                           const uint8_t salt[], size_t salt_len) = 0;

      virtual size_t agreed_value_size() const = 0;

      virtual ~Key_Agreement() = default;
   };

/**
* KEM (key encapsulation)
*/
class BOTAN_PUBLIC_API(2,0) KEM_Encryption
   {
   public:
      virtual void kem_encrypt(secure_vector<uint8_t>& out_encapsulated_key,
                               secure_vector<uint8_t>& out_shared_key,
                               size_t desired_shared_key_len,
                               Botan::RandomNumberGenerator& rng,
                               const uint8_t salt[],
                               size_t salt_len) = 0;

      virtual ~KEM_Encryption() = default;
   };

class BOTAN_PUBLIC_API(2,0) KEM_Decryption
   {
   public:
      virtual secure_vector<uint8_t> kem_decrypt(const uint8_t encap_key[],
                                              size_t len,
                                              size_t desired_shared_key_len,
                                              const uint8_t salt[],
                                              size_t salt_len) = 0;

      virtual ~KEM_Decryption() = default;
   };

}

}

#endif
