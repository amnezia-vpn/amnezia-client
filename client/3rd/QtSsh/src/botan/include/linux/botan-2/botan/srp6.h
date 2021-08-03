/*
* SRP-6a (RFC 5054 compatatible)
* (C) 2011,2012,2019 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RFC5054_SRP6_H_
#define BOTAN_RFC5054_SRP6_H_

#include <botan/bigint.h>
#include <botan/symkey.h>
#include <string>

namespace Botan {

class DL_Group;
class RandomNumberGenerator;

/**
* SRP6a Client side
* @param username the username we are attempting login for
* @param password the password we are attempting to use
* @param group_id specifies the shared SRP group
* @param hash_id specifies a secure hash function
* @param salt is the salt value sent by the server
* @param B is the server's public value
* @param rng is a random number generator
*
* @return (A,K) the client public key and the shared secret key
*/
std::pair<BigInt,SymmetricKey>
BOTAN_PUBLIC_API(2,0) srp6_client_agree(const std::string& username,
                            const std::string& password,
                            const std::string& group_id,
                            const std::string& hash_id,
                            const std::vector<uint8_t>& salt,
                            const BigInt& B,
                            RandomNumberGenerator& rng);


/**
* SRP6a Client side
* @param username the username we are attempting login for
* @param password the password we are attempting to use
* @param group specifies the shared SRP group
* @param hash_id specifies a secure hash function
* @param salt is the salt value sent by the server
* @param B is the server's public value
* @param a_bits size of secret exponent in bits
* @param rng is a random number generator
*
* @return (A,K) the client public key and the shared secret key
*/
std::pair<BigInt,SymmetricKey> BOTAN_PUBLIC_API(2,11)
   srp6_client_agree(const std::string& username,
                     const std::string& password,
                     const DL_Group& group,
                     const std::string& hash_id,
                     const std::vector<uint8_t>& salt,
                     const BigInt& B,
                     size_t a_bits,
                     RandomNumberGenerator& rng);

/**
* Generate a new SRP-6 verifier
* @param identifier a username or other client identifier
* @param password the secret used to authenticate user
* @param salt a randomly chosen value, at least 128 bits long
* @param group_id specifies the shared SRP group
* @param hash_id specifies a secure hash function
*/
BigInt BOTAN_PUBLIC_API(2,0)
   generate_srp6_verifier(const std::string& identifier,
                          const std::string& password,
                          const std::vector<uint8_t>& salt,
                          const std::string& group_id,
                          const std::string& hash_id);

/**
* Generate a new SRP-6 verifier
* @param identifier a username or other client identifier
* @param password the secret used to authenticate user
* @param salt a randomly chosen value, at least 128 bits long
* @param group specifies the shared SRP group
* @param hash_id specifies a secure hash function
*/
BigInt BOTAN_PUBLIC_API(2,11)
   generate_srp6_verifier(const std::string& identifier,
                          const std::string& password,
                          const std::vector<uint8_t>& salt,
                          const DL_Group& group,
                          const std::string& hash_id);

/**
* Return the group id for this SRP param set, or else thrown an
* exception
* @param N the group modulus
* @param g the group generator
* @return group identifier
*/
std::string BOTAN_PUBLIC_API(2,0) srp6_group_identifier(const BigInt& N, const BigInt& g);

/**
* Represents a SRP-6a server session
*/
class BOTAN_PUBLIC_API(2,0) SRP6_Server_Session final
   {
   public:
      /**
      * Server side step 1
      * @param v the verification value saved from client registration
      * @param group_id the SRP group id
      * @param hash_id the SRP hash in use
      * @param rng a random number generator
      * @return SRP-6 B value
      */
      BigInt step1(const BigInt& v,
                   const std::string& group_id,
                   const std::string& hash_id,
                   RandomNumberGenerator& rng);

      /**
      * Server side step 1
      * This version of step1 added in 2.11
      *
      * @param v the verification value saved from client registration
      * @param group the SRP group
      * @param hash_id the SRP hash in use
      * @param rng a random number generator
      * @param b_bits size of secret exponent in bits
      * @return SRP-6 B value
      */
      BigInt step1(const BigInt& v,
                   const DL_Group& group,
                   const std::string& hash_id,
                   const size_t b_bits,
                   RandomNumberGenerator& rng);

      /**
      * Server side step 2
      * @param A the client's value
      * @return shared symmetric key
      */
      SymmetricKey step2(const BigInt& A);

   private:
      std::string m_hash_id;
      BigInt m_B, m_b, m_v, m_S, m_p;
      size_t m_p_bytes = 0;
   };

}

#endif
