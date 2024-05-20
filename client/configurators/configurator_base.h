#ifndef CONFIGURATORBASE_H
#define CONFIGURATORBASE_H

#include <QObject>

#include "containers/containers_defs.h"
#include "core/defs.h"
#include "core/controllers/serverController.h"
#include "settings.h"

class ConfiguratorBase : public QObject
{
    Q_OBJECT
public:
    explicit ConfiguratorBase(std::shared_ptr<Settings> settings, const QSharedPointer<ServerController> &serverController, QObject *parent = nullptr);

    virtual QString createConfig(const ServerCredentials &credentials, DockerContainer container,
                                 const QJsonObject &containerConfig, ErrorCode &errorCode) = 0;

    virtual QString processConfigWithLocalSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                   QString &protocolConfigString);
    virtual QString processConfigWithExportSettings(const QPair<QString, QString> &dns, const bool isApiConfig,
                                                    QString &protocolConfigString);

protected:
    void processConfigWithDnsSettings(const QPair<QString, QString> &dns, QString &protocolConfigString);

    std::shared_ptr<Settings> m_settings;
    QSharedPointer<ServerController> m_serverController;

};

#endif // CONFIGURATORBASE_H
