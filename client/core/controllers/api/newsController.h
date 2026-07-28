#ifndef NEWSCONTROLLER_H
#define NEWSCONTROLLER_H

#include <QFuture>
#include <QJsonArray>
#include <QJsonObject>
#include <QPair>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureAppSettingsRepository.h"
#include "core/repositories/secureServersRepository.h"

class NewsController
{
public:
    explicit NewsController(SecureAppSettingsRepository* appSettingsRepository,
                           SecureServersRepository* serversRepository);

    QFuture<QPair<ErrorCode, QJsonArray>> fetchNews();

private:
    QJsonObject getServicesList() const;

    SecureAppSettingsRepository* m_appSettingsRepository;
    SecureServersRepository* m_serversRepository;
};

#endif // NEWSCONTROLLER_H
