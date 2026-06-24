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

// RNG с заранее заданными байтами — для детерминированных тестов.
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

} // namespace

int main()
{
    // --- base64: KAT (RFC 4648) -------------------------------------------
    CHECK_EQ(util::base64Encode(std::string("")), std::string(""));
    CHECK_EQ(util::base64Encode(std::string("f")), std::string("Zg=="));
    CHECK_EQ(util::base64Encode(std::string("fo")), std::string("Zm8="));
    CHECK_EQ(util::base64Encode(std::string("foo")), std::string("Zm9v"));
    CHECK_EQ(util::base64Encode(std::string("foob")), std::string("Zm9vYg=="));
    CHECK_EQ(util::base64Encode(std::string("fooba")), std::string("Zm9vYmE="));
    CHECK_EQ(util::base64Encode(std::string("foobar")), std::string("Zm9vYmFy"));

    // base64url без паддинга: байты 0xfb,0xff,0xbf → "-_-_" (стандарт дал бы "+/+/")
    {
        std::vector<std::uint8_t> v{0xfb, 0xff, 0xbf};
        CHECK_EQ(util::base64UrlEncodeNoPad(v), std::string("-_-_"));
        CHECK_EQ(util::base64Encode(v), std::string("+/+/"));
    }

    // round-trip decode (стандарт и url, с паддингом и без)
    {
        auto v = bytesOf("any carnal pleasure.");
        CHECK(util::base64Decode(util::base64Encode(v)) == v);
        CHECK(util::base64Decode(util::base64UrlEncodeNoPad(v)) == v);
    }

    // --- SHA-512 KAT: "abc" ------------------------------------------------
    CHECK_EQ(crypto::toHex(crypto::sha512(bytesOf("abc"))),
             std::string("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
                          "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"));

    // hex round-trip
    {
        std::vector<std::uint8_t> v{0x00, 0x01, 0xab, 0xff};
        CHECK_EQ(crypto::toHex(v), std::string("0001abff"));
        CHECK(crypto::fromHex("0001abff") == v);
    }

    // --- AES-256-CBC: эталон из openssl CLI -------------------------------
    {
        auto key = crypto::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        auto iv = crypto::fromHex("101112131415161718191a1b1c1d1e1f");
        auto pt = bytesOf("{\"hello\":\"world\"}");
        auto ct = crypto::aesEncryptCbc(pt, key, iv);
        CHECK_EQ(util::base64Encode(ct), std::string("2WHnAcP2N+l+jz7fbKyO46jjYUq7h98lxlTIT6K0Xg8="));
        // decrypt round-trip
        CHECK(crypto::aesDecryptCbc(ct, key, iv) == pt);
    }

    // IV: первые 16 байт 32-байтового iv дают тот же результат (паритет с оригиналом)
    {
        auto key = crypto::fromHex("000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f");
        auto iv16 = crypto::fromHex("101112131415161718191a1b1c1d1e1f");
        auto iv32 = crypto::fromHex("101112131415161718191a1b1c1d1e1fdeadbeefdeadbeefdeadbeefdeadbeef");
        auto pt = bytesOf("{\"hello\":\"world\"}");
        CHECK(crypto::aesEncryptCbc(pt, key, iv16) == crypto::aesEncryptCbc(pt, key, iv32));
    }

    // --- RSA PKCS1 v1.5 round-trip (тестовая пара) ------------------------
    {
        std::string pub = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_pub.pem");
        std::string priv = readFile(std::string(AGW_FIXTURES_DIR) + "/test_rsa_priv.pem");
        CHECK(!pub.empty());
        CHECK(!priv.empty());
        auto msg = bytesOf("{\"aes_key\":\"...\",\"aes_iv\":\"...\",\"aes_salt\":\"...\"}");
        auto ct = crypto::rsaEncryptPublicPkcs1(msg, pub);
        auto rt = crypto::rsaDecryptPrivatePkcs1(ct, priv);
        CHECK(rt == msg);
        // недетерминированность паддинга: два шифрования дают разные байты
        auto ct2 = crypto::rsaEncryptPublicPkcs1(msg, pub);
        CHECK(ct != ct2);
    }

    // --- UUID v4 формат ----------------------------------------------------
    {
        // 16 байт 0xFF: проверяем выставление version/variant и форму 8-4-4-4-12
        FixedRng rng(std::vector<std::uint8_t>(16, 0xFF));
        std::string u = util::makeUuidV4(rng);
        CHECK_EQ(u, std::string("ffffffff-ffff-4fff-bfff-ffffffffffff"));
        CHECK(u.size() == 36);
        CHECK(u[14] == '4');                         // версия
        CHECK(u[19] == '8' || u[19] == '9' || u[19] == 'a' || u[19] == 'b'); // вариант
    }

    return AGW_TEST_MAIN_RETURN();
}
