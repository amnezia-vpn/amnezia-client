/*
* EMSA Classes
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PUBKEY_EMSA_H_
#define BOTAN_PUBKEY_EMSA_H_

#include <botan/secmem.h>
#include <botan/asn1_obj.h>
#include <string>

BOTAN_FUTURE_INTERNAL_HEADER(emsa.h)

namespace Botan {

class Private_Key;
class RandomNumberGenerator;

/**
* EMSA, from IEEE 1363s Encoding Method for Signatures, Appendix
*
* Any way of encoding/padding signatures
*/
class BOTAN_PUBLIC_API(2,0) EMSA
   {
   public:
      virtual ~EMSA() = default;

      /**
      * Add more data to the signature computation
      * @param input some data
      * @param length length of input in bytes
      */
      virtual void update(const uint8_t input[], size_t length) = 0;

      /**
      * @return raw hash
      */
      virtual secure_vector<uint8_t> raw_data() = 0;

      /**
      * Return the encoding of a message
      * @param msg the result of raw_data()
      * @param output_bits the desired output bit size
      * @param rng a random number generator
      * @return encoded signature
      */
      virtual secure_vector<uint8_t> encoding_of(const secure_vector<uint8_t>& msg,
                                             size_t output_bits,
                                             RandomNumberGenerator& rng) = 0;

      /**
      * Verify the encoding
      * @param coded the received (coded) message representative
      * @param raw the computed (local, uncoded) message representative
      * @param key_bits the size of the key in bits
      * @return true if coded is a valid encoding of raw, otherwise false
      */
      virtual bool verify(const secure_vector<uint8_t>& coded,
                          const secure_vector<uint8_t>& raw,
                          size_t key_bits) = 0;

      /**
      * Prepare sig_algo for use in choose_sig_format for x509 certs
      *
      * @param key used for checking compatibility with the encoding scheme
      * @param cert_hash_name is checked to equal the hash for the encoding
      * @return algorithm identifier to signatures created using this key,
      *         padding method and hash.
      */
      virtual AlgorithmIdentifier config_for_x509(const Private_Key& key,
                                                  const std::string& cert_hash_name) const;

      /**
      * @return a new object representing the same encoding method as *this
      */
      virtual EMSA* clone() = 0;

      /**
      * @return the SCAN name of the encoding/padding scheme
      */
      virtual std::string name() const = 0;
   };

/**
* Factory method for EMSA (message-encoding methods for signatures
* with appendix) objects
* @param algo_spec the name of the EMSA to create
* @return pointer to newly allocated object of that type
*/
BOTAN_PUBLIC_API(2,0) EMSA* get_emsa(const std::string& algo_spec);

/**
* Returns the hash function used in the given EMSA scheme
* If the hash function is not specified or not understood,
* returns "SHA-512"
* @param algo_spec the name of the EMSA
* @return hash function used in the given EMSA scheme
*/
BOTAN_PUBLIC_API(2,0) std::string hash_for_emsa(const std::string& algo_spec);

}

#endif
