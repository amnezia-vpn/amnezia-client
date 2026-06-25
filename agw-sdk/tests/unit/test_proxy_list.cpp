#include "agw_test.h"

#include <string>
#include <vector>

#include "crypto/aes.h"
#include "crypto/hash.h"
#include "failover/proxy_list.h"
#include "util/base64.h"

using namespace agw;

namespace {
std::vector<std::uint8_t> bytesOf(const std::string &s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
}
} // namespace

int main()
{
    // --- buildStorageUrls: порядок и пути -----------------------------------
    {
        const std::vector<std::string> primary{"https://a/", "https://b/"};
        const std::vector<std::string> fallback{"https://f/"};
        const FailoverContext ctx{"prem", "US"};

        const std::string enc =
            util::base64UrlEncodeNoPad(bytesOf("endpoints-prem-US"));

        const auto urls = failover::buildStorageUrls(primary, fallback, ctx);
        const std::vector<std::string> expected{
            "https://a/" + enc + ".json",
            "https://b/" + enc + ".json",
            "https://a/endpoints.json",
            "https://b/endpoints.json",
            "https://f/" + enc + ".json",
            "https://f/endpoints.json",
        };
        CHECK(urls == expected);
    }

    // пустой service → только generic
    {
        const std::vector<std::string> primary{"https://a/", "https://b/"};
        const std::vector<std::string> fallback{"https://f/"};
        const FailoverContext ctx{"", ""};
        const auto urls = failover::buildStorageUrls(primary, fallback, ctx);
        const std::vector<std::string> expected{
            "https://a/endpoints.json",
            "https://b/endpoints.json",
            "https://f/endpoints.json",
        };
        CHECK(urls == expected);
    }

    // --- decodeProxyList dev: открытый JSON-массив --------------------------
    {
        const auto list = failover::decodeProxyList(R"(["https://p1/","https://p2/"])", /*dev=*/true, "");
        const std::vector<std::string> expected{"https://p1/", "https://p2/"};
        CHECK(list == expected);
        // не массив → пусто
        CHECK(failover::decodeProxyList(R"({"x":1})", true, "").empty());
    }

    // --- decodeProxyList prod: self-consistent (ключ/IV из SHA-512(pubkey)) -
    {
        const std::string pub = "PUBKEYDATA-pem-like";
        const std::string h = crypto::toHex(crypto::sha512(bytesOf(pub)));
        const auto key = crypto::fromHex(h.substr(0, 64));
        const auto iv = crypto::fromHex(h.substr(64, 32));

        const std::string arr = R"(["https://prod1/","https://prod2/"])";
        const auto cipher = crypto::aesEncryptCbc(bytesOf(arr), key, iv);
        const std::string b64 = util::base64Encode(cipher);

        const auto list = failover::decodeProxyList(b64, /*dev=*/false, pub);
        const std::vector<std::string> expected{"https://prod1/", "https://prod2/"};
        CHECK(list == expected);

        // мусор вместо base64-шифртекста → throw
        bool threw = false;
        try {
            failover::decodeProxyList("###not base64 cipher###", false, pub);
        } catch (...) {
            threw = true;
        }
        CHECK(threw);
    }

    return AGW_TEST_MAIN_RETURN();
}
