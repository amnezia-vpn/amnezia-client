#ifndef NEWSCONTROLLER_H
#define NEWSCONTROLLER_H

#include <QFuture>
#include <QJsonArray>
#include <QPair>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/controllers/serversController.h"

class NewsController
{
public:
    explicit NewsController(SecureAppSettingsRepository* appSettingsRepository,
                           ServersController* serversController);

    QFuture<QPair<ErrorCode, QJsonArray>> fetchNews();

private:
    SecureAppSettingsRepository* m_appSettingsRepository;
    ServersController* m_serversController;
};

#endif // NEWSCONTROLLER_H

