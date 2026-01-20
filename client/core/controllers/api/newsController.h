#ifndef NEWSCONTROLLER_H
#define NEWSCONTROLLER_H

#include <QFuture>
#include <QJsonArray>
#include <QPair>

#include "core/utils/defs.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "core/controllers/serversController.h"

class NewsController
{
public:
    explicit NewsController(QAppSettingsRepository* appSettingsRepository,
                           ServersController* serversController);

    QFuture<QPair<ErrorCode, QJsonArray>> fetchNews();

private:
    QAppSettingsRepository* m_appSettingsRepository;
    ServersController* m_serversController;
};

#endif // NEWSCONTROLLER_H

