#include "agw_test.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "crypto/aes.h"
#include "crypto/hash.h"
#include "crypto/rsa.h"
#include "protocol/keys.h"
#include "util/base64.h"
#include "util/json.h"

using namespace agw;

namespace {
std::vector<std::uint8_t> bytesOf(const std::string &s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

std::string toStr(const std::vector<std::uint8_t> &v)
{
    return std::string(v.begin(), v.end());
}

std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}
}

int main()
{
    namespace k = protocol::keys;

    const auto key = crypto::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
    const auto iv = crypto::fromHex("101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c2d2e2f");
    const auto salt = crypto::fromHex("a0a1a2a3a4a5a6a7");
    const std::string payload = "{\"hello\":\"world\"}";

    util::Json keysJson;
    keysJson[k::aesKey] = util::base64Encode(key);
    keysJson[k::aesIv] = util::base64Encode(iv);
    keysJson[k::aesSalt] = util::base64Encode(salt);
    const std::string keysSerialized = util::qtIndentedDump(keysJson);

    const std::string expectedKeysJson =
        "{\n"
        "    \"aes_iv\": \"EBESExQVFhcYGRobHB0eHyAhIiMkJSYnKCkqKywtLi8=\",\n"
        "    \"aes_key\": \"AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8=\",\n"
        "    \"aes_salt\": \"oKGio6Slpqc=\"\n"
        "}\n";
    CHECK_EQ(keysSerialized, expectedKeysJson);

    const auto apiCipher = crypto::aesEncryptCbc(bytesOf(payload), key, iv);
    const std::string apiPayloadB64 = util::base64Encode(apiCipher);
    CHECK_EQ(apiPayloadB64, std::string("2WHnAcP2N+l+jz7fbKyO46jjYUq7h98lxlTIT6K0Xg8="));

    const std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
    const std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
    CHECK(!pub.empty());
    CHECK(!priv.empty());

    const auto keyCipher = crypto::rsaEncryptPublicPkcs1(bytesOf(keysSerialized), pub);
    const std::string keyPayloadB64 = util::base64Encode(keyCipher);

    const auto keyCipherBack = util::base64Decode(keyPayloadB64);
    const auto recovered = crypto::rsaDecryptPrivatePkcs1(keyCipherBack, priv);
    CHECK_EQ(toStr(recovered), keysSerialized);

    util::Json body;
    body[k::keyPayload] = keyPayloadB64;
    body[k::apiPayload] = apiPayloadB64;
    const std::string bodySerialized = util::qtIndentedDump(body);

    util::Json parsed = util::Json::parse(bodySerialized);
    CHECK_EQ(parsed[k::apiPayload].get<std::string>(), apiPayloadB64);
    {
        const auto cBack = util::base64Decode(parsed[k::keyPayload].get<std::string>());
        const auto rec = crypto::rsaDecryptPrivatePkcs1(cBack, priv);
        CHECK_EQ(toStr(rec), keysSerialized);
    }

    {
        const auto respPlain = bytesOf("{\"ok\":true}");
        const auto respCipher = crypto::aesEncryptCbc(respPlain, key, iv);
        const auto back = crypto::aesDecryptCbc(respCipher, key, iv);
        CHECK(back == respPlain);
    }

    return AGW_TEST_MAIN_RETURN();
}
