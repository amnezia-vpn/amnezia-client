/*
* Bcrypt Password Hashing
* (C) 2011 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_BCRYPT_H_
#define BOTAN_BCRYPT_H_

#include <botan/types.h>
#include <string>

namespace Botan {

class RandomNumberGenerator;

/**
* Create a password hash using Bcrypt
*
* @warning The password is truncated at at most 72 characters; characters after
*          that do not have any effect on the resulting hash. To support longer
*          passwords, consider pre-hashing the password, for example by using
*          the hex encoding of SHA-256 of the password as the input to bcrypt.
*
* @param password the password.
* @param rng a random number generator
* @param work_factor how much work to do to slow down guessing attacks
* @param version which version to emit (may be 'a', 'b', or 'y' all of which
*        have identical behavior in this implementation).
*
* @see https://www.usenix.org/events/usenix99/provos/provos_html/
*/
std::string BOTAN_PUBLIC_API(2,0) generate_bcrypt(const std::string& password,
                                                  RandomNumberGenerator& rng,
                                                  uint16_t work_factor = 12,
                                                  char version = 'a');

/**
* Check a previously created password hash
* @param password the password to check against
* @param hash the stored hash to check against
*/
bool BOTAN_PUBLIC_API(2,0) check_bcrypt(const std::string& password,
                                        const std::string& hash);

}

#endif
