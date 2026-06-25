#ifndef AGW_DETAIL_TEST_HOOKS_H
#define AGW_DETAIL_TEST_HOOKS_H

#include <memory>

#include "agw/http.h"

// Тест-хук (НЕ часть публичного API): подменить транспорт для следующего agw_client_create,
// чтобы гонять C-ABI через in-process mock без сети. В боевом пути не используется.
namespace agw::detail {

void setNextTestHttpClient(std::shared_ptr<IHttpClient> http);
std::shared_ptr<IHttpClient> takeNextTestHttpClient();

} // namespace agw::detail

#endif // AGW_DETAIL_TEST_HOOKS_H
