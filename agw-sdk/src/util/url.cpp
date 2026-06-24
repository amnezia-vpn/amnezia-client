#include "util/url.h"

namespace agw::util {

std::string formatEndpoint(const std::string &endpoint, const std::string &host)
{
    const std::string token = "%1";
    const std::size_t pos = endpoint.find(token);
    if (pos == std::string::npos) {
        return endpoint;
    }
    std::string out = endpoint;
    out.replace(pos, token.size(), host);
    return out;
}

std::string extractHost(const std::string &url)
{
    std::size_t start = 0;
    const std::size_t scheme = url.find("://");
    if (scheme != std::string::npos) {
        start = scheme + 3;
    }

    // конец authority — до первого '/', '?' или '#'
    std::size_t end = url.size();
    for (std::size_t i = start; i < url.size(); ++i) {
        const char c = url[i];
        if (c == '/' || c == '?' || c == '#') {
            end = i;
            break;
        }
    }

    std::string authority = url.substr(start, end - start);

    // отбросить userinfo
    const std::size_t at = authority.find('@');
    if (at != std::string::npos) {
        authority = authority.substr(at + 1);
    }

    // отбросить порт (без поддержки IPv6 в скобках — для шлюза не требуется)
    const std::size_t colon = authority.find(':');
    if (colon != std::string::npos) {
        authority = authority.substr(0, colon);
    }

    return authority;
}

} // namespace agw::util
