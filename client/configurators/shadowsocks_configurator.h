#ifndef SHADOWSOCKS_CONFIGURATOR_H
#define SHADOWSOCKS_CONFIGURATOR_H

#include <QObject>

#include "configurator_base.h"
#include "core/defs.h"

class ShadowSocksConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    ShadowSocksConfigurator(std::shared_ptr<Settings> settings, QObject *parent = nullptr);

    QString createConfig(const ServerCredentials &credentials, DockerContainer container,
                         const QJsonObject &containerConfig, QString &clientId, ErrorCode errorCode);
};

#endif // SHADOWSOCKS_CONFIGURATOR_H
