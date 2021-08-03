/*
* CECPQ1 (x25519 + NewHope)
* (C) 2016 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CECPQ1_H_
#define BOTAN_CECPQ1_H_

#include <botan/secmem.h>
#include <botan/newhope.h>

namespace Botan {

class CECPQ1_key final
   {
   public:
      secure_vector<uint8_t> m_x25519;
      newhope_poly m_newhope;
   };

void BOTAN_PUBLIC_API(2,0) CECPQ1_offer(uint8_t* offer_message,
                            CECPQ1_key* offer_key_output,
                            RandomNumberGenerator& rng);

void BOTAN_PUBLIC_API(2,0) CECPQ1_accept(uint8_t* shared_key,
                             uint8_t* accept_message,
                             const uint8_t* offer_message,
                             RandomNumberGenerator& rng);

void BOTAN_PUBLIC_API(2,0) CECPQ1_finish(uint8_t* shared_key,
                             const CECPQ1_key& offer_key,
                             const uint8_t* accept_message);

}

#endif
