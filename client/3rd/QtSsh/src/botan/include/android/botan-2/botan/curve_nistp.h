/*
* Arithmetic operations specialized for NIST ECC primes
* (C) 2014,2015 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_NIST_PRIMES_H_
#define BOTAN_NIST_PRIMES_H_

#include <botan/bigint.h>

BOTAN_FUTURE_INTERNAL_HEADER(curve_nistp.h)

namespace Botan {

/**
* NIST Prime reduction functions.
*
* Reduces the value in place
*
* ws is a workspace function which is used as a temporary,
* and will be resized as needed.
*/
BOTAN_PUBLIC_API(2,0) const BigInt& prime_p521();
BOTAN_PUBLIC_API(2,0) void redc_p521(BigInt& x, secure_vector<word>& ws);

/*
Previously this macro indicated if the P-{192,224,256,384} reducers
were available. Now they are always enabled and this macro has no meaning.
The define will be removed in a future major release.
*/
#define BOTAN_HAS_NIST_PRIME_REDUCERS_W32

BOTAN_PUBLIC_API(2,0) const BigInt& prime_p384();
BOTAN_PUBLIC_API(2,0) void redc_p384(BigInt& x, secure_vector<word>& ws);

BOTAN_PUBLIC_API(2,0) const BigInt& prime_p256();
BOTAN_PUBLIC_API(2,0) void redc_p256(BigInt& x, secure_vector<word>& ws);

BOTAN_PUBLIC_API(2,0) const BigInt& prime_p224();
BOTAN_PUBLIC_API(2,0) void redc_p224(BigInt& x, secure_vector<word>& ws);

BOTAN_PUBLIC_API(2,0) const BigInt& prime_p192();
BOTAN_PUBLIC_API(2,0) void redc_p192(BigInt& x, secure_vector<word>& ws);

}

#endif
