#include "agw_test.h"

#include <string>

#include "util/json.h"

using namespace agw;

int main()
{
    {
        util::Json j;
        j["aes_key"] = "KEY";
        j["aes_iv"] = "IV";
        j["aes_salt"] = "SALT";

        const std::string expected =
            "{\n"
            "    \"aes_iv\": \"IV\",\n"
            "    \"aes_key\": \"KEY\",\n"
            "    \"aes_salt\": \"SALT\"\n"
            "}\n";
        CHECK_EQ(util::qtIndentedDump(j), expected);
    }

    {
        util::Json j;
        j["key_payload"] = "K";
        j["api_payload"] = "A";
        const std::string expected =
            "{\n"
            "    \"api_payload\": \"A\",\n"
            "    \"key_payload\": \"K\"\n"
            "}\n";
        CHECK_EQ(util::qtIndentedDump(j), expected);
    }

    {
        util::Json j;
        j["s"] = std::string("a\"b\\c\nd\te\x01");
        const std::string expected =
            "{\n"
            "    \"s\": \"a\\\"b\\\\c\\nd\\te\\u0001\"\n"
            "}\n";
        CHECK_EQ(util::qtIndentedDump(j), expected);
    }

    {
        util::Json j;
        j["outer"]["inner"] = "v";
        const std::string expected =
            "{\n"
            "    \"outer\": {\n"
            "        \"inner\": \"v\"\n"
            "    }\n"
            "}\n";
        CHECK_EQ(util::qtIndentedDump(j), expected);
    }

    return AGW_TEST_MAIN_RETURN();
}
