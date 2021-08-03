/*
* RTSS (threshold secret sharing)
* (C) 2009,2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_RTSS_H_
#define BOTAN_RTSS_H_

#include <botan/secmem.h>
#include <string>
#include <vector>

namespace Botan {

class RandomNumberGenerator;

/**
* A split secret, using the format from draft-mcgrew-tss-03
*/
class BOTAN_PUBLIC_API(2,0) RTSS_Share final
   {
   public:
      /**
      * @param M the number of shares needed to reconstruct
      * @param N the number of shares generated
      * @param secret the secret to split
      * @param secret_len the length of the secret
      * @param identifier the 16 byte share identifier
      * @param rng the random number generator to use
      */
      static std::vector<RTSS_Share>
         split(uint8_t M, uint8_t N,
               const uint8_t secret[], uint16_t secret_len,
               const uint8_t identifier[16],
               RandomNumberGenerator& rng);

      /**
      * @param M the number of shares needed to reconstruct
      * @param N the number of shares generated
      * @param secret the secret to split
      * @param secret_len the length of the secret
      * @param identifier the share identifier
      * @param hash_fn the hash function to use for a checksum ("None", "SHA-1", "SHA-256")
      * @param rng the random number generator to use
      */
      static std::vector<RTSS_Share>
         split(uint8_t M, uint8_t N,
               const uint8_t secret[], uint16_t secret_len,
               const std::vector<uint8_t>& identifier,
               const std::string& hash_fn,
               RandomNumberGenerator& rng);

      /**
      * @param shares the list of shares
      */
      static secure_vector<uint8_t>
        reconstruct(const std::vector<RTSS_Share>& shares);

      RTSS_Share() = default;

      /**
      * @param hex_input the share encoded in hexadecimal
      */
      explicit RTSS_Share(const std::string& hex_input);

      /**
      * @param data the shared data
      * @param len the length of data
      */
      RTSS_Share(const uint8_t data[], size_t len);

      /**
      * @return binary representation
      */
      const secure_vector<uint8_t>& data() const { return m_contents; }

      /**
      * @return hex representation
      */
      std::string to_string() const;

      /**
      * @return share identifier
      */
      uint8_t share_id() const;

      /**
      * @return size of this share in bytes
      */
      size_t size() const { return m_contents.size(); }

      /**
      * @return if this TSS share was initialized or not
      */
      bool initialized() const { return (m_contents.size() > 0); }
   private:
      secure_vector<uint8_t> m_contents;
   };

}

#endif
