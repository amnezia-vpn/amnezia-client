#ifndef AGW_UTIL_URL_H
#define AGW_UTIL_URL_H

#include <string>

namespace agw::util
{
    std::string formatEndpoint(const std::string &endpoint, const std::string &host);

    std::string extractHost(const std::string &url);
}

#endif
