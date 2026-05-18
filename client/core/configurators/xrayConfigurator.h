#ifndef XRAY_CONFIGURATOR_H
#define XRAY_CONFIGURATOR_H

#include <QObject>
#include <QJsonObject>

#include "configuratorBase.h"
#include "core/utils/errorCodes.h"
#include "core/utils/routeModes.h"
#include "core/utils/commonStructs.h"
#include "core/models/protocols/xrayProtocolConfig.h"

class XrayConfigurator : public ConfiguratorBase
{
    Q_OBJECT
public:
    XrayConfigurator(SshSession* sshSession, QObject *parent = nullptr);

    amnezia::ProtocolConfig createConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container, const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode) override;

    amnezia::ProtocolConfig processConfigWithLocalSettings(const amnezia::ConnectionSettings &settings,
                                                           amnezia::ProtocolConfig protocolConfig) override;

private:
    QString prepareServerConfig(const amnezia::ServerCredentials &credentials, amnezia::DockerContainer container, const amnezia::ContainerConfig &containerConfig,
                                const amnezia::DnsSettings &dnsSettings,
                                amnezia::ErrorCode &errorCode);

    // Builds the native xray "streamSettings" JSON object from XrayServerConfig
    QJsonObject buildStreamSettings(const amnezia::XrayServerConfig &srv,
                                    const QString &clientId) const;
};

#endif // XRAY_CONFIGURATOR_H
