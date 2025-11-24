#ifndef SERVICESCATALOGCONTROLLER_H
#define SERVICESCATALOGCONTROLLER_H

#include <QJsonObject>
#include <QByteArray>

#include "core/defs.h"
#include "settings.h"

class ServicesCatalogController
{
public:
    explicit ServicesCatalogController(std::shared_ptr<Settings> settings);

    ErrorCode fillAvailableServices(QJsonObject &servicesData);

private:
    ErrorCode executeRequest(const QString &endpoint, const QJsonObject &apiPayload, QByteArray &responseBody);

    std::shared_ptr<Settings> m_settings;
};

#endif // SERVICESCATALOGCONTROLLER_H

