#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>
#include <QList>
#include <QPair>
#include <QSharedPointer>

#include "core/models/containers/containers_defs.h"
#include "core/defs.h"
#include "core/controllers/selfhosted/serverController.h"
#include "core/models/protocols/protocolConfig.h"
#include "settings.h"

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    using Vars = QList<QPair<QString, QString>>;

    explicit ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    virtual QSharedPointer<ProtocolConfig> createConfig(const ServerCredentials &credentials, DockerContainer container,
                                                        const QSharedPointer<ProtocolConfig> &protocolConfig, ErrorCode &errorCode) = 0;

    virtual QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                   QString &protocolConfigString);
    virtual QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                    QString &protocolConfigString);

    virtual Vars generateProtocolVars(const ServerCredentials &credentials, DockerContainer container,
                                     const QSharedPointer<ProtocolConfig> &protocolConfig) const;

protected:
    void processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString);

    Vars generateCommonVars(const ServerCredentials &credentials, DockerContainer container) const;

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;

};

#endif // CONFIGURATORBASE_H
