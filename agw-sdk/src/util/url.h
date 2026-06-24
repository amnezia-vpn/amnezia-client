#ifndef AGW_UTIL_URL_H
#define AGW_UTIL_URL_H

#include <string>

namespace agw::util {

// Заменяет первое вхождение "%1" на host (паритет с endpoint.arg(host)).
std::string formatEndpoint(const std::string &endpoint, const std::string &host);

// Извлекает host из URL (scheme://[user@]host[:port]/path → host), как QUrl::host().
std::string extractHost(const std::string &url);

} // namespace agw::util

#endif // AGW_UTIL_URL_H
