#ifndef AGW_UTIL_BASE64_H
#define AGW_UTIL_BASE64_H

#include <cstdint>
#include <string>
#include <vector>

namespace agw::util
{
    std::string base64Encode(const std::vector<std::uint8_t> &data);

    std::string base64UrlEncodeNoPad(const std::vector<std::uint8_t> &data);

    std::vector<std::uint8_t> base64Decode(const std::string &text);

    std::string base64Encode(const std::string &data);
}

#endif
