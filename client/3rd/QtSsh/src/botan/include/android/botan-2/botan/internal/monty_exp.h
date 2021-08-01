/*
* (C) 2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MONTY_EXP_H_
#define BOTAN_MONTY_EXP_H_

#include <memory>

namespace Botan {

class BigInt;
class Modular_Reducer;

class Montgomery_Params;

class Montgomery_Exponentation_State;

/*
* Precompute for calculating values g^x mod p
*/
std::shared_ptr<const Montgomery_Exponentation_State>
monty_precompute(std::shared_ptr<const Montgomery_Params> params_p,
                 const BigInt& g,
                 size_t window_bits,
                 bool const_time = true);

/*
* Return g^k mod p
*/
BigInt monty_execute(const Montgomery_Exponentation_State& precomputed_state,
                     const BigInt& k, size_t max_k_bits);

/*
* Return g^k mod p taking variable time depending on k
* @warning only use this if k is public
*/
BigInt monty_execute_vartime(const Montgomery_Exponentation_State& precomputed_state,
                             const BigInt& k);

/**
* Return (x^z1 * y^z2) % p
*/
BigInt monty_multi_exp(std::shared_ptr<const Montgomery_Params> params_p,
                       const BigInt& x,
                       const BigInt& z1,
                       const BigInt& y,
                       const BigInt& z2);

}

#endif
