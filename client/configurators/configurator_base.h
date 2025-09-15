#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>
#include <QSharedPointer>

#include "containers/containers_defs.h"
#include "core/controllers/serverController.h"
#include "core/defs.h"
#include "core/models/containers/containerConfig.h"
#include "core/models/protocols/protocolConfig.h"
#include "core/models/servers/selfHostedServerConfig.h"
#include "settings.h"

class ProtocolConfig;

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController,
                              QObject *parent = nullptr);

    virtual QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &serverCredentials, const ContainerConfig &containerConfig,
                                                        ErrorCode &errorCode) = 0;

    virtual QSharedPointer<ProtocolConfig> processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                                          QSharedPointer<ProtocolConfig> protocolConfig);
    virtual QSharedPointer<ProtocolConfig> processConfigWithExportSettings(const QPair<QString, QString> &dns,
                                                                           QSharedPointer<ProtocolConfig> protocolConfig);

protected:
    void processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString);
    void updateNativeConfig(QSharedPointer<ProtocolConfig> protocolConfig, const QString &nativeConfig);

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;
};

#endif // CONFIGURATORBASE_H
