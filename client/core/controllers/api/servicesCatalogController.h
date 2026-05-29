#ifndef SERVICESCATALOGCONTROLLER_H
#define SERVICESCATALOGCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/repositories/secureAppSettingsRepository.h"

class ServicesCatalogController
{
public:
    explicit ServicesCatalogController(SecureAppSettingsRepository* appSettingsRepository);

    ErrorCode fillAvailableServices(QJsonObject &servicesData);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    SecureAppSettingsRepository* m_appSettingsRepository;
};

#endif // SERVICESCATALOGCONTROLLER_H

