/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_FFI_PKEY_H_
#define BOTAN_FFI_PKEY_H_

#include <botan/pk_keys.h>
#include <botan/internal/ffi_util.h>

extern "C" {

BOTAN_FFI_DECLARE_STRUCT(botan_pubkey_struct, Botan::Public_Key, 0x2C286519);
BOTAN_FFI_DECLARE_STRUCT(botan_privkey_struct, Botan::Private_Key, 0x7F96385E);

}

#endif
