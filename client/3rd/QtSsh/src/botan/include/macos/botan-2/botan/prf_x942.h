/*
* X9.42 PRF
* (C) 1999-2007 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_ANSI_X942_PRF_H_
#define BOTAN_ANSI_X942_PRF_H_

#include <botan/kdf.h>
#include <botan/asn1_obj.h>

BOTAN_FUTURE_INTERNAL_HEADER(prf_x942.h)

namespace Botan {

/**
* PRF from ANSI X9.42
*/
class BOTAN_PUBLIC_API(2,0) X942_PRF final : public KDF
   {
   public:
      std::string name() const override;

      KDF* clone() const override { return new X942_PRF(m_key_wrap_oid); }

      size_t kdf(uint8_t key[], size_t key_len,
                 const uint8_t secret[], size_t secret_len,
                 const uint8_t salt[], size_t salt_len,
                 const uint8_t label[], size_t label_len) const override;

      explicit X942_PRF(const std::string& oid) : m_key_wrap_oid(OID::from_string(oid)) {}

      explicit X942_PRF(const OID& oid) : m_key_wrap_oid(oid) {}
   private:
      OID m_key_wrap_oid;
   };

}

#endif
