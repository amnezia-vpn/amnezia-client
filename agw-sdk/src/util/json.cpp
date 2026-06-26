#include "json.h"

#include <cstdint>

namespace agw::util
{
    namespace
    {
        const char *kHex = "0123456789abcdef";

        void appendEscaped(std::string &out, const std::string &s)
        {
            out.push_back('"');
            for (unsigned char c : s) {
                switch (c) {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20) {
                        out += "\\u00";
                        out.push_back(kHex[c >> 4]);
                        out.push_back(kHex[c & 0x0F]);
                    } else {
                        out.push_back(static_cast<char>(c));
                    }
                }
            }
            out.push_back('"');
        }

        void appendIndent(std::string &out, int level)
        {
            out.append(static_cast<std::size_t>(level) * 4, ' ');
        }

        void dumpValue(std::string &out, const Json &j, int indent);

        void dumpObject(std::string &out, const Json &j, int indent)
        {
            out += "{\n";
            const int inner = indent + 1;
            std::size_t i = 0;
            const std::size_t n = j.size();
            for (auto it = j.begin(); it != j.end(); ++it, ++i) {
                appendIndent(out, inner);
                appendEscaped(out, it.key());
                out += ": ";
                dumpValue(out, it.value(), inner);
                if (i + 1 < n) {
                    out.push_back(',');
                }
                out.push_back('\n');
            }
            appendIndent(out, indent);
            out.push_back('}');
        }

        void dumpArray(std::string &out, const Json &j, int indent)
        {
            out += "[\n";
            const int inner = indent + 1;
            std::size_t i = 0;
            const std::size_t n = j.size();
            for (const auto &el : j) {
                appendIndent(out, inner);
                dumpValue(out, el, inner);
                if (i + 1 < n) {
                    out.push_back(',');
                }
                out.push_back('\n');
                ++i;
            }
            appendIndent(out, indent);
            out.push_back(']');
        }

        void dumpValue(std::string &out, const Json &j, int indent)
        {
            switch (j.type()) {
            case Json::value_t::object: dumpObject(out, j, indent); break;
            case Json::value_t::array: dumpArray(out, j, indent); break;
            case Json::value_t::string: appendEscaped(out, j.get<std::string>()); break;
            case Json::value_t::boolean: out += j.get<bool>() ? "true" : "false"; break;
            case Json::value_t::null: out += "null"; break;
            case Json::value_t::number_integer:
            case Json::value_t::number_unsigned:
            case Json::value_t::number_float:
            default:

                out += j.dump();
                break;
            }
        }
    }

    std::string qtIndentedDump(const Json &j)
    {
        std::string out;
        dumpValue(out, j, 0);
        out.push_back('\n');
        return out;
    }
}
