#include "agw_test.h"

#include <string>

#include "failover/bypass_policy.h"

using namespace agw;
using agw::failover::shouldBypassProxy;

namespace {
bool bypassBody(const std::string &body)
{
    return shouldBypassProxy(TransportError::None, body, /*ok=*/true);
}
} // namespace

int main()
{
    // провал расшифровки → байпас
    CHECK(shouldBypassProxy(TransportError::None, "garbage", /*ok=*/false) == true);

    // таймаут/отмена → байпас
    CHECK(shouldBypassProxy(TransportError::Timeout, R"({"http_status":200})", true) == true);
    CHECK(shouldBypassProxy(TransportError::Canceled, R"({"http_status":200})", true) == true);

    // html в теле → байпас
    CHECK(bypassBody("<html><body>blocked</body></html>") == true);

    // 408/409/402 → НЕ байпас
    CHECK(bypassBody(R"({"http_status":408})") == false);
    CHECK(bypassBody(R"({"http_status":409})") == false);
    CHECK(bypassBody(R"({"http_status":402})") == false);

    // 404: без паттерна → байпас; с паттерном → не байпас
    CHECK(bypassBody(R"({"http_status":404,"message":"whatever"})") == true);
    CHECK(bypassBody(R"({"http_status":404,"message":"No active configuration found for x"})") == false);
    CHECK(bypassBody(R"({"http_status":404,"detail":"Account not found."})") == false);
    CHECK(bypassBody(R"({"http_status":404,"message":"Session not found"})") == false);

    // 501: без паттерна → байпас; с паттерном обновления → не байпас
    CHECK(bypassBody(R"({"http_status":501})") == true);
    CHECK(bypassBody(R"({"http_status":501,"message":"client version update is required"})") == false);

    // 422: строка про подписку → не байпас; иначе → байпас
    CHECK(bypassBody(R"({"http_status":422,"message":"Failed to retrieve subscription information. Is it activated?"})") == false);
    CHECK(bypassBody(R"({"http_status":422,"message":"other"})") == true);

    // чистый успех (200, NoError) → не байпас
    CHECK(bypassBody(R"({"http_status":200})") == false);
    // не-JSON тело + NoError → не байпас
    CHECK(bypassBody("plain ok") == false);
    // транспортная ошибка (не timeout) + валидное тело без спец-статуса → байпас
    CHECK(shouldBypassProxy(TransportError::ConnectionError, R"({"http_status":200})", true) == true);

    return AGW_TEST_MAIN_RETURN();
}
