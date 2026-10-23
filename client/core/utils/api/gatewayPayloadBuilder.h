#ifndef GATEWAYPAYLOADBUILDER_H
#define GATEWAYPAYLOADBUILDER_H

#include <QJsonObject>
#include <QJsonValue>
#include <QLatin1String>

class SecureAppSettingsRepository;

class GatewayPayloadBuilder
{
public:
    explicit GatewayPayloadBuilder(const SecureAppSettingsRepository *appSettingsRepository);

    GatewayPayloadBuilder &addField(QLatin1String key, const QJsonValue &value);
    QJsonObject build() const;

private:
    QJsonObject m_payload;
};

#endif // GATEWAYPAYLOADBUILDER_H
