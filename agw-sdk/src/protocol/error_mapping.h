#ifndef AGW_PROTOCOL_ERROR_MAPPING_H
#define AGW_PROTOCOL_ERROR_MAPPING_H

#include <string>

#include "agw/http.h"
#include "agw/types.h"

namespace agw::protocol {

// Перенос apiUtils::checkNetworkReplyErrors один в один.
// http_status берётся из ПОЛЯ тела JSON (decryptedBody), а не из фактического HTTP-кода.
ErrorCode mapResponseError(bool sslError, TransportError transportError, const std::string &decryptedBody);

} // namespace agw::protocol

#endif // AGW_PROTOCOL_ERROR_MAPPING_H
