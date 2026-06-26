#ifndef AGW_PROTOCOL_ERROR_MAPPING_H
#define AGW_PROTOCOL_ERROR_MAPPING_H

#include <string>

#include "agw/http.h"
#include "agw/types.h"

namespace agw::protocol
{
    ErrorCode mapResponseError(bool sslError, TransportError transportError, const std::string &decryptedBody);
}

#endif
