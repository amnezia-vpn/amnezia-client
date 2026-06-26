#ifndef AGW_FAILOVER_BYPASS_POLICY_H
#define AGW_FAILOVER_BYPASS_POLICY_H

#include <string>

#include "agw/http.h"

namespace agw::failover
{

    // Перенос GatewayController::shouldBypassProxy один в один.
    // Уходим на прокси при: провале расшифровки; таймауте/отмене; подстроке "html" в теле;
    // http_status ∈ {404,501} кроме известных строк-паттернов; 422 кроме строки про подписку;
    // прочей транспортной ошибке. Не байпасим при 408/409/402 и при чистом NoError.
    //
    // Параметры: транспортная ошибка, расшифрованное тело, успешность расшифровки.
    // SSL-ошибка проверяется вызывающим ОТДЕЛЬНО (как в оригинале: bypass только при пустых sslErrors).
    bool shouldBypassProxy(TransportError transportError, const std::string &decryptedBody, bool decryptionSuccessful);

} // namespace agw::failover

#endif // AGW_FAILOVER_BYPASS_POLICY_H
