/*
* (C) 2018 Ribose Inc
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_PWDHASH_H_
#define BOTAN_PWDHASH_H_

#include <botan/types.h>
#include <string>
#include <memory>
#include <vector>
#include <chrono>

namespace Botan {

/**
* Base class for password based key derivation functions.
*
* Converts a password into a key using a salt and iterated hashing to
* make brute force attacks harder.
*/
class BOTAN_PUBLIC_API(2,8) PasswordHash
   {
   public:
      virtual ~PasswordHash() = default;

      virtual std::string to_string() const = 0;

      /**
      * Most password hashes have some notion of iterations.
      */
      virtual size_t iterations() const = 0;

      /**
      * Some password hashing algorithms have a parameter which controls how
      * much memory is used. If not supported by some algorithm, returns 0.
      */
      virtual size_t memory_param() const { return 0; }

      /**
      * Some password hashing algorithms have a parallelism parameter.
      * If the algorithm does not support this notion, then the
      * function returns zero. This allows distinguishing between a
      * password hash which just does not support parallel operation,
      * vs one that does support parallel operation but which has been
      * configured to use a single lane.
      */
      virtual size_t parallelism() const { return 0; }

      /**
      * Returns an estimate of the total memory usage required to perform this
      * key derivation.
      *
      * If this algorithm uses a small and constant amount of memory, with no
      * effort made towards being memory hard, this function returns 0.
      */
      virtual size_t total_memory_usage() const { return 0; }

      /**
      * Derive a key from a password
      *
      * @param out buffer to store the derived key, must be of out_len bytes
      * @param out_len the desired length of the key to produce
      * @param password the password to derive the key from
      * @param password_len the length of password in bytes
      * @param salt a randomly chosen salt
      * @param salt_len length of salt in bytes
      *
      * This function is const, but is not thread safe. Different threads should
      * either use unique objects, or serialize all access.
      */
      virtual void derive_key(uint8_t out[], size_t out_len,
                              const char* password, size_t password_len,
                              const uint8_t salt[], size_t salt_len) const = 0;
   };

class BOTAN_PUBLIC_API(2,8) PasswordHashFamily
   {
   public:
      /**
      * Create an instance based on a name
      * If provider is empty then best available is chosen.
      * @param algo_spec algorithm name
      * @param provider provider implementation to choose
      * @return a null pointer if the algo/provider combination cannot be found
      */
      static std::unique_ptr<PasswordHashFamily> create(const std::string& algo_spec,
                                                        const std::string& provider = "");

      /**
      * Create an instance based on a name, or throw if the
      * algo/provider combination cannot be found. If provider is
      * empty then best available is chosen.
      */
      static std::unique_ptr<PasswordHashFamily>
         create_or_throw(const std::string& algo_spec,
                         const std::string& provider = "");

      /**
      * @return list of available providers for this algorithm, empty if not available
      */
      static std::vector<std::string> providers(const std::string& algo_spec);

      virtual ~PasswordHashFamily() = default;

      /**
      * @return name of this PasswordHash
      */
      virtual std::string name() const = 0;

      /**
      * Return a new parameter set tuned for this machine
      * @param output_length how long the output length will be
      * @param msec the desired execution time in milliseconds
      *
      * @param max_memory_usage_mb some password hash functions can use a tunable
      * amount of memory, in this case max_memory_usage limits the amount of RAM
      * the returned parameters will require, in mebibytes (2**20 bytes). It may
      * require some small amount above the request. Set to zero to place no
      * limit at all.
      */
      virtual std::unique_ptr<PasswordHash> tune(size_t output_length,
                                                 std::chrono::milliseconds msec,
                                                 size_t max_memory_usage_mb = 0) const = 0;

      /**
      * Return some default parameter set for this PBKDF that should be good
      * enough for most users. The value returned may change over time as
      * processing power and attacks improve.
      */
      virtual std::unique_ptr<PasswordHash> default_params() const = 0;

      /**
      * Return a parameter chosen based on a rough approximation with the
      * specified iteration count. The exact value this returns for a particular
      * algorithm may change from over time. Think of it as an alternative to
      * tune, where time is expressed in terms of PBKDF2 iterations rather than
      * milliseconds.
      */
      virtual std::unique_ptr<PasswordHash> from_iterations(size_t iterations) const = 0;

      /**
      * Create a password hash using some scheme specific format.
      * Eg PBKDF2 and PGP-S2K set iterations in i1
      * Scrypt uses N,r,p in i{1-3}
      * Bcrypt-PBKDF just has iterations
      * Argon2{i,d,id} would use iterations, memory, parallelism for i{1-3},
      * and Argon2 type is part of the family.
      *
      * Values not needed should be set to 0
      */
      virtual std::unique_ptr<PasswordHash> from_params(
         size_t i1,
         size_t i2 = 0,
         size_t i3 = 0) const = 0;
   };

}

#endif
