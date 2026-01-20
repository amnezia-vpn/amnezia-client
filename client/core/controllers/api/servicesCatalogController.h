#ifndef SERVICESCATALOGCONTROLLER_H
#define SERVICESCATALOGCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/qAppSettingsRepository.h"

class ServicesCatalogController
{
public:
    explicit ServicesCatalogController(QAppSettingsRepository* appSettingsRepository);

    ErrorCode fillAvailableServices(QJsonObject &servicesData);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    QAppSettingsRepository* m_appSettingsRepository;
};

#endif // SERVICESCATALOGCONTROLLER_H

