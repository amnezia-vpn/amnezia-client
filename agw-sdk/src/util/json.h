#ifndef AGW_UTIL_JSON_H
#define AGW_UTIL_JSON_H

#include <string>

#include <nlohmann/json.hpp>

namespace agw::util
{
    using Json = nlohmann::json;

    std::string qtIndentedDump(const Json &j);
}

#endif
