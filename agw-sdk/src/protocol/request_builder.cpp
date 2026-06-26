#include "protocol/request_builder.h"

#include "crypto/aes.h"
#include "crypto/rsa.h"
#include "protocol/keys.h"
#include "util/base64.h"
#include "util/json.h"

namespace agw::protocol
{

    namespace
    {

        std::vector<std::uint8_t> bytesOf(const std::string &s)
        {
            return std::vector<std::uint8_t>(s.begin(), s.end());
        }

    } // namespace

    EncryptedRequest buildEncryptedRequest(const std::string &payload, const std::string &publicKeyPem, crypto::IRng &rng)
    {
        namespace k = keys;

        EncryptedRequest out;
        out.key = rng.bytes(32);
        out.iv = rng.bytes(32);
        out.salt = rng.bytes(8);

        // Невалидный ключ — отдельная ветка до шифрования (как в оригинале).
        if (!crypto::rsaPublicKeyValid(publicKeyPem)) {
            out.error = ErrorCode::ApiMissingAgwPublicKey;
            return out;
        }

        util::Json keysJson;
        keysJson[k::aesKey] = util::base64Encode(out.key);
        keysJson[k::aesIv] = util::base64Encode(out.iv);
        keysJson[k::aesSalt] = util::base64Encode(out.salt);
        const std::string keysSerialized = util::qtIndentedDump(keysJson);

        std::string keyPayloadB64;
        std::string apiPayloadB64;
        try {
            keyPayloadB64 = util::base64Encode(crypto::rsaEncryptPublicPkcs1(bytesOf(keysSerialized), publicKeyPem));
            apiPayloadB64 = util::base64Encode(crypto::aesEncryptCbc(bytesOf(payload), out.key, out.iv));
        } catch (...) {
            out.error = ErrorCode::ApiConfigDecryptionError;
            return out;
        }

        util::Json body;
        body[k::keyPayload] = keyPayloadB64;
        body[k::apiPayload] = apiPayloadB64;
        out.body = util::qtIndentedDump(body);
        return out;
    }

} // namespace agw::protocol
