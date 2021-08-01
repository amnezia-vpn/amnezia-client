/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FFI_MP_H_
#define BOTAN_FFI_MP_H_

#include <botan/bigint.h>
#include <botan/internal/ffi_util.h>

extern "C" {

BOTAN_FFI_DECLARE_STRUCT(botan_mp_struct, Botan::BigInt, 0xC828B9D2);

}

#endif
