/*
* (C) 2017 Jack Lloyd
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_TLS_ALGO_IDS_H_
#define BOTAN_TLS_ALGO_IDS_H_

#include <botan/types.h>
#include <string>
#include <vector>

//BOTAN_FUTURE_INTERNAL_HEADER(tls_algos.h)

namespace Botan {

namespace TLS {

enum class Cipher_Algo {
   CHACHA20_POLY1305,

   AES_128_CBC_HMAC_SHA1 = 100,
   AES_128_CBC_HMAC_SHA256,
   AES_128_CCM,
   AES_128_CCM_8,
   AES_128_GCM,
   AES_128_OCB,

   AES_256_CBC_HMAC_SHA1 = 200,
   AES_256_CBC_HMAC_SHA256,
   AES_256_CBC_HMAC_SHA384,
   AES_256_CCM,
   AES_256_CCM_8,
   AES_256_GCM,
   AES_256_OCB,

   CAMELLIA_128_CBC_HMAC_SHA1 = 300,
   CAMELLIA_128_CBC_HMAC_SHA256,
   CAMELLIA_128_GCM,

   CAMELLIA_256_CBC_HMAC_SHA1 = 400,
   CAMELLIA_256_CBC_HMAC_SHA256,
   CAMELLIA_256_CBC_HMAC_SHA384,
   CAMELLIA_256_GCM,

   ARIA_128_GCM = 500,
   ARIA_256_GCM,

   DES_EDE_CBC_HMAC_SHA1 = 1000,
   SEED_CBC_HMAC_SHA1,
};

enum class KDF_Algo {
   SHA_1,
   SHA_256,
   SHA_384,
};

std::string BOTAN_DLL kdf_algo_to_string(KDF_Algo algo);

enum class Nonce_Format {
   CBC_MODE,
   AEAD_IMPLICIT_4,
   AEAD_XOR_12,
};

// TODO encoding should match signature_algorithms extension
// TODO this should include hash etc as in TLS v1.3
enum class Auth_Method {
   RSA,
   DSA,
   ECDSA,

   // These are placed outside the encodable range
   IMPLICIT = 0x10000,
   ANONYMOUS
};

std::string BOTAN_TEST_API auth_method_to_string(Auth_Method method);
Auth_Method BOTAN_TEST_API auth_method_from_string(const std::string& str);

/*
* This matches the wire encoding
*/
enum class Signature_Scheme : uint16_t {
   NONE             = 0x0000,

   RSA_PKCS1_SHA1   = 0x0201,
   RSA_PKCS1_SHA256 = 0x0401,
   RSA_PKCS1_SHA384 = 0x0501,
   RSA_PKCS1_SHA512 = 0x0601,

   DSA_SHA1   = 0x0202,
   DSA_SHA256 = 0x0402,
   DSA_SHA384 = 0x0502,
   DSA_SHA512 = 0x0602,

   ECDSA_SHA1   = 0x0203,
   ECDSA_SHA256 = 0x0403,
   ECDSA_SHA384 = 0x0503,
   ECDSA_SHA512 = 0x0603,

   RSA_PSS_SHA256 = 0x0804,
   RSA_PSS_SHA384 = 0x0805,
   RSA_PSS_SHA512 = 0x0806,

   EDDSA_25519 = 0x0807,
   EDDSA_448   = 0x0808,
};

BOTAN_UNSTABLE_API const std::vector<Signature_Scheme>& all_signature_schemes();

bool BOTAN_UNSTABLE_API signature_scheme_is_known(Signature_Scheme scheme);
std::string BOTAN_UNSTABLE_API sig_scheme_to_string(Signature_Scheme scheme);
std::string BOTAN_UNSTABLE_API hash_function_of_scheme(Signature_Scheme scheme);
std::string BOTAN_UNSTABLE_API padding_string_for_scheme(Signature_Scheme scheme);
std::string signature_algorithm_of_scheme(Signature_Scheme scheme);

/*
* Matches with wire encoding
*/
enum class Group_Params : uint16_t {
   NONE = 0,

   SECP256R1 = 23,
   SECP384R1 = 24,
   SECP521R1 = 25,
   BRAINPOOL256R1 = 26,
   BRAINPOOL384R1 = 27,
   BRAINPOOL512R1 = 28,

   X25519 = 29,

   FFDHE_2048 = 256,
   FFDHE_3072 = 257,
   FFDHE_4096 = 258,
   FFDHE_6144 = 259,
   FFDHE_8192 = 260,
};

std::string group_param_to_string(Group_Params group);
Group_Params group_param_from_string(const std::string& group_name);
bool group_param_is_dh(Group_Params group);

enum class Kex_Algo {
   STATIC_RSA,
   DH,
   ECDH,
   CECPQ1,
   SRP_SHA,
   PSK,
   DHE_PSK,
   ECDHE_PSK,
};

std::string BOTAN_TEST_API kex_method_to_string(Kex_Algo method);
Kex_Algo BOTAN_TEST_API kex_method_from_string(const std::string& str);

inline bool key_exchange_is_psk(Kex_Algo m)
   {
   return (m == Kex_Algo::PSK ||
           m == Kex_Algo::DHE_PSK ||
           m == Kex_Algo::ECDHE_PSK);
   }

}

}

#endif
