#ifndef AGW_FAILOVER_BYPASS_POLICY_H
#define AGW_FAILOVER_BYPASS_POLICY_H

#include <string>

#include "agw/http.h"

namespace agw::failover
{
    bool shouldBypassProxy(TransportError transportError, const std::string &decryptedBody, bool decryptionSuccessful);
}

#endif
