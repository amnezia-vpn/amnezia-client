#include "agw_test.h"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "crypto/aes.h"
#include "crypto/hash.h"
#include "crypto/rng.h"
#include "crypto/rsa.h"
#include "util/base64.h"
#include "util/uuid.h"

using namespace agw;

namespace {
std::vector<std::uint8_t> bytesOf(const std::string &s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
}

std::string readFile(const std::string &path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

class FixedRng : public crypto::IRng {
public:
    explicit FixedRng(std::vector<std::uint8_t> data) : m_data(std::move(data)) {}
    std::vector<std::uint8_t> bytes(std::size_t n) override
    {
        std::vector<std::uint8_t> out(n);
        for (std::size_t i = 0; i < n; ++i) {
            out[i] = m_data[(m_pos + i) % m_data.size()];
        }
        m_pos += n;
        return out;
    }
private:
    std::vector<std::uint8_t> m_data;
    std::size_t m_pos = 0;
};
}

int main()
{
    CHECK_EQ(util::base64Encode(std::string("")), std::string(""));
    CHECK_EQ(util::base64Encode(std::string("f")), std::string("Zg=="));
    CHECK_EQ(util::base64Encode(std::string("fo")), std::string("Zm8="));
    CHECK_EQ(util::base64Encode(std::string("foo")), std::string("Zm9v"));
    CHECK_EQ(util::base64Encode(std::string("foob")), std::string("Zm9vYg=="));
    CHECK_EQ(util::base64Encode(std::string("fooba")), std::string("Zm9vYmE="));
    CHECK_EQ(util::base64Encode(std::string("foobar")), std::string("Zm9vYmFy"));

    {
        std::vector<std::uint8_t> v{0xfb, 0xff, 0xbf};
        CHECK_EQ(util::base64UrlEncodeNoPad(v), std::string("-_-_"));
        CHECK_EQ(util::base64Encode(v), std::string("+/+/"));
    }

    {
        auto v = bytesOf("any carnal pleasure.");
        CHECK(util::base64Decode(util::base64Encode(v)) == v);
        CHECK(util::base64Decode(util::base64UrlEncodeNoPad(v)) == v);
    }

    CHECK_EQ(crypto::toHex(crypto::sha512(bytesOf("abc"))),
             std::string("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                          "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));

    {
        std::vector<std::uint8_t> v{0x00, 0x01, 0xab, 0xff};
        CHECK_EQ(crypto::toHex(v), std::string("0001abff"));
        CHECK(crypto::fromHex("0001abff") == v);
    }

    {
        auto key = crypto::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        auto iv = crypto::fromHex("101112131415161718191a1b1c1d1e1f");
        auto pt = bytesOf("{\"hello\":\"world\"}");
        auto ct = crypto::aesEncryptCbc(pt, key, iv);
        CHECK_EQ(util::base64Encode(ct), std::string("2WHnAcP2N+l+jz7fbKyO46jjYUq7h98lxlTIT6K0Xg8="));

        CHECK(crypto::aesDecryptCbc(ct, key, iv) == pt);
    }

    {
        auto key = crypto::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        auto iv16 = crypto::fromHex("101112131415161718191a1b1c1d1e1f");
        auto iv32 = crypto::fromHex("101112131415161718191a1b1c1d1e1fdeadbeefdeadbeefdeadbeefdeadbeef");
        auto pt = bytesOf("{\"hello\":\"world\"}");
        CHECK(crypto::aesEncryptCbc(pt, key, iv16) == crypto::aesEncryptCbc(pt, key, iv32));
    }

    {
        std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
        std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
        CHECK(!pub.empty());
        CHECK(!priv.empty());
        auto msg = bytesOf("{\"aes_key\":\"...\",\"aes_iv\":\"...\",\"aes_salt\":\"...\"}");
        auto ct = crypto::rsaEncryptPublicPkcs1(msg, pub);
        auto rt = crypto::rsaDecryptPrivatePkcs1(ct, priv);
        CHECK(rt == msg);

        auto ct2 = crypto::rsaEncryptPublicPkcs1(msg, pub);
        CHECK(ct != ct2);
    }

    {
        FixedRng rng(std::vector<std::uint8_t>(16, 0xFF));
        std::string u = util::makeUuidV4(rng);
        CHECK_EQ(u, std::string("ffffffff-ffff-4fff-bfff-ffffffffffff"));
        CHECK(u.size() == 36);
        CHECK(u[14] == '4');
        CHECK(u[19] == '8' || u[19] == '9' || u[19] == 'a' || u[19] == 'b');
    }

    return AGW_TEST_MAIN_RETURN();
}
