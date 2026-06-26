#include "base64.h"

#include <array>

namespace agw::util
{
    namespace
    {
        const char *kStd = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        const char *kUrl = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

        std::string encode(const std::vector<std::uint8_t> &data, const char *alphabet, bool pad)
        {
            std::string out;
            out.reserve((data.size() + 2) / 3 * 4);

            std::size_t i = 0;
            const std::size_t n = data.size();
            while (i + 3 <= n) {
                const std::uint32_t v = (std::uint32_t(data[i]) << 16) | (std::uint32_t(data[i + 1]) << 8) | data[i + 2];
                out.push_back(alphabet[(v >> 18) & 0x3F]);
                out.push_back(alphabet[(v >> 12) & 0x3F]);
                out.push_back(alphabet[(v >> 6) & 0x3F]);
                out.push_back(alphabet[v & 0x3F]);
                i += 3;
            }

            const std::size_t rem = n - i;
            if (rem == 1) {
                const std::uint32_t v = std::uint32_t(data[i]) << 16;
                out.push_back(alphabet[(v >> 18) & 0x3F]);
                out.push_back(alphabet[(v >> 12) & 0x3F]);
                if (pad) {
                    out.push_back('=');
                    out.push_back('=');
                }
            } else if (rem == 2) {
                const std::uint32_t v = (std::uint32_t(data[i]) << 16) | (std::uint32_t(data[i + 1]) << 8);
                out.push_back(alphabet[(v >> 18) & 0x3F]);
                out.push_back(alphabet[(v >> 12) & 0x3F]);
                out.push_back(alphabet[(v >> 6) & 0x3F]);
                if (pad) {
                    out.push_back('=');
                }
            }
            return out;
        }

        int decodeChar(char c)
        {
            if (c >= 'A' && c <= 'Z')
                return c - 'A';
            if (c >= 'a' && c <= 'z')
                return c - 'a' + 26;
            if (c >= '0' && c <= '9')
                return c - '0' + 52;
            if (c == '+' || c == '-')
                return 62;
            if (c == '/' || c == '_')
                return 63;
            return -1;
        }
    }

    std::string base64Encode(const std::vector<std::uint8_t> &data)
    {
        return encode(data, kStd, true);
    }

    std::string base64UrlEncodeNoPad(const std::vector<std::uint8_t> &data)
    {
        return encode(data, kUrl, false);
    }
    std::string base64UrlEncode(const std::vector<std::uint8_t> &data)
    {
        return encode(data, kUrl, true);
    }

    std::string base64Encode(const std::string &data)
    {
        return base64Encode(std::vector<std::uint8_t>(data.begin(), data.end()));
    }

    std::vector<std::uint8_t> base64Decode(const std::string &text)
    {
        std::vector<std::uint8_t> out;
        out.reserve(text.size() / 4 * 3 + 3);

        std::array<int, 4> quad { };
        int count = 0;
        for (char c : text) {
            const int v = decodeChar(c);
            if (v < 0) {
                continue;
            }
            quad[count++] = v;
            if (count == 4) {
                out.push_back(static_cast<std::uint8_t>((quad[0] << 2) | (quad[1] >> 4)));
                out.push_back(static_cast<std::uint8_t>((quad[1] << 4) | (quad[2] >> 2)));
                out.push_back(static_cast<std::uint8_t>((quad[2] << 6) | quad[3]));
                count = 0;
            }
        }
        if (count == 2) {
            out.push_back(static_cast<std::uint8_t>((quad[0] << 2) | (quad[1] >> 4)));
        } else if (count == 3) {
            out.push_back(static_cast<std::uint8_t>((quad[0] << 2) | (quad[1] >> 4)));
            out.push_back(static_cast<std::uint8_t>((quad[1] << 4) | (quad[2] >> 2)));
        }
        return out;
    }
}
