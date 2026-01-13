#ifndef NEWSCONTROLLER_H
#define NEWSCONTROLLER_H

#include <QJsonArray>
#include <QByteArray>

#include "core/utils/defs.h"
#include "core/repositories/appSettingsRepository.h"
#include "core/controllers/serversController.h"

class NewsController
{
public:
    explicit NewsController(AppSettingsRepository* appSettingsRepository,
                           ServersController* serversController);

    ErrorCode fetchNews(QJsonArray &newsArray);

private:
    AppSettingsRepository* m_appSettingsRepository;
    ServersController* m_serversController;
};

#endif // NEWSCONTROLLER_H

