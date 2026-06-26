#ifndef AGW_DETAIL_TEST_HOOKS_H
#define AGW_DETAIL_TEST_HOOKS_H

#include <memory>

#include "agw/http.h"

namespace agw::detail
{
    void setNextTestHttpClient(std::shared_ptr<IHttpClient> http);
    std::shared_ptr<IHttpClient> takeNextTestHttpClient();
}

#endif
