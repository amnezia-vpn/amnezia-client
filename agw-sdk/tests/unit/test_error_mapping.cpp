#include "agw_test.h"

#include <string>

#include "protocol/error_mapping.h"

using namespace agw;
using agw::protocol::mapResponseError;

namespace {

int code(ErrorCode e) { return static_cast<int>(e); }

ErrorCode mapBody(const std::string &body)
{
    return mapResponseError(false, TransportError::None, body);
}

} // namespace

int main()
{
    // транспортные/ssl ветки (тело пустое)
    CHECK(mapResponseError(true, TransportError::None, "") == ErrorCode::ApiConfigSslError);
    CHECK(mapResponseError(false, TransportError::Timeout, "") == ErrorCode::ApiConfigTimeoutError);
    CHECK(mapResponseError(false, TransportError::Canceled, "") == ErrorCode::ApiConfigTimeoutError);
    CHECK(mapResponseError(false, TransportError::OperationNotImplemented, "") == ErrorCode::ApiUpdateRequestError);
    CHECK(mapResponseError(false, TransportError::ConnectionError, "") == ErrorCode::ApiConfigDownloadError);
    CHECK(mapResponseError(false, TransportError::None, "") == ErrorCode::NoError);

    // не-JSON тело + NoError → NoError
    CHECK(mapBody("not a json") == ErrorCode::NoError);

    // http_status из тела
    CHECK(mapBody(R"({"http_status":429})") == ErrorCode::ApiRateLimitError);
    CHECK(mapBody(R"({"http_status":409})") == ErrorCode::ApiConfigLimitError);
    CHECK(mapBody(R"({"http_status":409,"message":"Trial Subscription Already Used"})") == ErrorCode::ApiTrialAlreadyUsedError);
    CHECK(mapBody(R"({"http_status":404})") == ErrorCode::ApiNotFoundError);
    CHECK(mapBody(R"({"http_status":408})") == ErrorCode::ApiConfigTimeoutError);
    CHECK(mapBody(R"({"http_status":501})") == ErrorCode::ApiUpdateRequestError);

    // 422
    CHECK(mapBody(R"({"http_status":422,"message":"Failed to retrieve subscription information. Is it activated?"})")
          == ErrorCode::ApiSubscriptionExpiredError);
    CHECK(mapBody(R"({"http_status":422,"message":"something else"})") == ErrorCode::ApiConfigDownloadError);

    // 402 — каптча и подписка
    CHECK(mapBody(R"({"http_status":402,"message":"refresh_captcha"})") == ErrorCode::ApiCaptchaRefreshError);
    CHECK(mapBody(R"({"http_status":402,"message":"invalid_captcha"})") == ErrorCode::ApiCaptchaInvalidError);
    CHECK(mapBody(R"({"http_status":402,"captcha_id":"x"})") == ErrorCode::ApiCaptchaRequiredError);
    CHECK(mapBody(R"({"http_status":402,"captcha_image":"x"})") == ErrorCode::ApiCaptchaRequiredError);
    CHECK(mapBody(R"({"http_status":402,"message":"rate_limit_exceeded"})") == ErrorCode::ApiCaptchaRequiredError);
    CHECK(mapBody(R"({"http_status":402,"message":"nope"})") == ErrorCode::ApiSubscriptionNotActiveError);

    // прочий >=300
    CHECK(mapBody(R"({"http_status":500})") == ErrorCode::ApiConfigDownloadError);
    // <300 и NoError → NoError
    CHECK(mapBody(R"({"http_status":200})") == ErrorCode::NoError);

    (void)code;
    return AGW_TEST_MAIN_RETURN();
}
