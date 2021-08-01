/*
* (C) 2018 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_MP_MONTY_H_
#define BOTAN_MP_MONTY_H_

#include <botan/types.h>

namespace Botan {

/*
* Each of these functions makes the following assumptions:
*
* z_size >= 2*(p_size + 1)
* ws_size >= z_size
*/

void bigint_monty_redc_4(word z[], const word p[], word p_dash, word ws[]);
void bigint_monty_redc_6(word z[], const word p[], word p_dash, word ws[]);
void bigint_monty_redc_8(word z[], const word p[], word p_dash, word ws[]);
void bigint_monty_redc_16(word z[], const word p[], word p_dash, word ws[]);
void bigint_monty_redc_24(word z[], const word p[], word p_dash, word ws[]);
void bigint_monty_redc_32(word z[], const word p[], word p_dash, word ws[]);


}

#endif
