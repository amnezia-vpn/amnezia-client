#include "agw_test.h"

#include <string>

#include "failover/bypass_policy.h"

using namespace agw;
using agw::failover::shouldBypassProxy;

namespace {
bool bypassBody(const std::string &body)
{
    return shouldBypassProxy(TransportError::None, body, true);
}
}

int main()
{
    CHECK(shouldBypassProxy(TransportError::None, "garbage", false) == true);

    CHECK(shouldBypassProxy(TransportError::Timeout, R"({"http_status":200})", true) == true);
    CHECK(shouldBypassProxy(TransportError::Canceled, R"({"http_status":200})", true) == true);

    CHECK(bypassBody("<html><body>blocked</body></html>") == true);

    CHECK(bypassBody(R"({"http_status":408})") == false);
    CHECK(bypassBody(R"({"http_status":409})") == false);
    CHECK(bypassBody(R"({"http_status":402})") == false);

    CHECK(bypassBody(R"({"http_status":404,"message":"whatever"})") == true);
    CHECK(bypassBody(R"({"http_status":404,"message":"No active configuration found for x"})") == false);
    CHECK(bypassBody(R"({"http_status":404,"detail":"Account not found."})") == false);
    CHECK(bypassBody(R"({"http_status":404,"message":"Session not found"})") == false);

    CHECK(bypassBody(R"({"http_status":501})") == true);
    CHECK(bypassBody(R"({"http_status":501,"message":"client version update is required"})") == false);

    CHECK(bypassBody(R"({"http_status":422,"message":"Failed to retrieve subscription information. Is it activated?"})") == false);
    CHECK(bypassBody(R"({"http_status":422,"message":"other"})") == true);

    CHECK(bypassBody(R"({"http_status":200})") == false);

    CHECK(bypassBody("plain ok") == false);

    CHECK(shouldBypassProxy(TransportError::ConnectionError, R"({"http_status":200})", true) == true);

    return AGW_TEST_MAIN_RETURN();
}
