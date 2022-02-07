#ifndef VPN_CONFIGURATOR_H
#define VPN_CONFIGURATOR_H

#include <QObject>

#include "core/defs.h"
#include "settings.h"
#include "core/servercontroller.h"

// Retrieve connection settings from server
class VpnConfigurator
{
public:

    static QString genVpnProtocolConfig(const ServerCredentials &credentials, DockerContainer container,
        const QJsonObject &containerConfig, Proto proto, ErrorCode *errorCode = nullptr);

    static QPair<QString, QString> getDnsForConfig(int serverIndex);
    static QString &processConfigWithDnsSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);

    static QString &processConfigWithLocalSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);
    static QString &processConfigWithExportSettings(int serverIndex, DockerContainer container, Proto proto, QString &config);

    // workaround for containers which is not support normal configaration
    static void updateContainerConfigAfterInstallation(DockerContainer container,
        QJsonObject &containerConfig, const QString &stdOut);

    static Settings &m_settings();
};

#endif // VPN_CONFIGURATOR_H
