#ifndef SERVICESCATALOGCONTROLLER_H
#define SERVICESCATALOGCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/defs.h"
#include "core/repositories/appSettingsRepository.h"

class ServicesCatalogController
{
public:
    explicit ServicesCatalogController(AppSettingsRepository* appSettingsRepository);

    ErrorCode fillAvailableServices(QJsonObject &servicesData);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    AppSettingsRepository* m_appSettingsRepository;
};

#endif // SERVICESCATALOGCONTROLLER_H

