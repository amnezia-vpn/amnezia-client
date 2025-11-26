#ifndef SERVICESCATALOGCONTROLLER_H
#define SERVICESCATALOGCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/defs.h"
#include "core/repositories/appSettingsRepository.h"

class ServicesCatalogController
{
public:
    explicit ServicesCatalogController(std::shared_ptr<AppSettingsRepository> appSettingsRepository);

    ErrorCode fillAvailableServices(QJsonObject &servicesData);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    std::shared_ptr<AppSettingsRepository> m_appSettingsRepository;
};

#endif // SERVICESCATALOGCONTROLLER_H

