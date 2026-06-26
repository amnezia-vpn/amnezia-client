#include "agw_test.h"

#include <ctime>
#include <string>
#include <vector>

#include "crypto/hash.h"
#include "services/subscription.h"
#include "services/vpn_key.h"
#include "util/json.h"
#include "util/zlib.h"

using namespace agw;

namespace {
std::vector<std::uint8_t> bytesOf(const std::string &s)
{
    return std::vector<std::uint8_t>(s.begin(), s.end());
}
} // namespace

int main()
{
    // --- zlib: формат qCompress (4-байтовый BE-префикс + zlib-поток) ------
    // эталон из python zlib (level 6): данные {"hello":"world"} (17 байт)
    {
        const auto data = bytesOf("{\"hello\":\"world\"}");
        const auto c = util::qtCompress(data, 6);
        CHECK_EQ(crypto::toHex(c),
                 std::string("00000011789cab56ca48cdc9c957b2522acf2fca4951aa0500356b05f7"));
        // round-trip
        CHECK(util::qtUncompress(c) == data);
    }
    // round-trip на более крупных данных
    {
        std::string big;
        for (int i = 0; i < 500; ++i) big += "abcABC123-";
        const auto data = bytesOf(big);
        CHECK(util::qtUncompress(util::qtCompress(data, 6)) == data);
    }

    // --- vpn-ключ V1: байт-в-байт с эталоном --------------------------------
    {
        util::Json sc;
        sc["name"] = "Srv";
        sc["description"] = "desc";
        sc["config_version"] = 1.0;
        sc["protocol"] = "awg";
        sc["api_endpoint"] = "https://ep/";
        sc["api_key"] = "KEY";
        CHECK_EQ(services::buildPremiumV1VpnKey(sc),
                 std::string("vpn://AAAA_3icNcoxC4NADIbhvyKZpdeu7k4dOznJcUYbWpNwFxQR_3sbxPF73m8HjjNCU8ErL1BXMGBJmdRI2NWncxIeaeoXzOUsj9v9z5rFJMnXr3Gd_BmVeuRBhdic32ZamhBQw5U_uHl5th0cP1k0KEs="));
    }

    // --- vpn-ключ V2: байт-в-байт с эталоном --------------------------------
    {
        util::Json sc;
        sc["name"] = "Srv2";
        sc["description"] = "d2";
        sc["config_version"] = 2.0;
        sc["api_config"] = {{"service_type", "amnezia-premium"},
                            {"service_protocol", "awg"},
                            {"user_country_code", "US"}};
        sc["auth_data"] = {{"api_key", "AK"}};
        CHECK_EQ(services::buildPremiumV2VpnKey(sc),
                 std::string("vpn://AAAA_3icNY29DsIwDAZfpfIMS0c2ZsaKObISUyzIj5ykKFR5d5JUTJburPt2cGgJLhMsss1wmsBQ1MIhsXcdmwG1dw9e1UYSDz43iIHVIRrYIZJsrEmlEkYQraMv4zkIWc62Z_4vQXzy2r_H22ftKjfXatklKe2akbgvUPtQTk9lMOHY6bMvKt1fb1DrD3EXQEw="));
    }

    // --- подписка: парсинг ISO + expired/expiringSoon -----------------------
    {
        std::time_t end = 0;
        CHECK(services::parseIso8601Utc("2026-06-25T00:00:00Z", end));

        // смещение зоны приводится к UTC (03:00+03:00 == 00:00Z)
        std::time_t endOff = 0;
        CHECK(services::parseIso8601Utc("2026-06-25T03:00:00+03:00", endOff));
        CHECK(end == endOff);

        CHECK(services::isSubscriptionExpired("2026-06-25T00:00:00Z", end + 1) == true);
        CHECK(services::isSubscriptionExpired("2026-06-25T00:00:00Z", end - 1) == false);
        CHECK(services::isSubscriptionExpired("", 123456) == false);
        CHECK(services::isSubscriptionExpired("garbage", 123456) == false);

        CHECK(services::isSubscriptionExpiringSoon("2026-06-25T00:00:00Z", end - 10 * 86400, 30) == true);
        CHECK(services::isSubscriptionExpiringSoon("2026-06-25T00:00:00Z", end - 40 * 86400, 30) == false);
        CHECK(services::isSubscriptionExpiringSoon("2026-06-25T00:00:00Z", end + 1, 30) == false);
    }

    return AGW_TEST_MAIN_RETURN();
}
