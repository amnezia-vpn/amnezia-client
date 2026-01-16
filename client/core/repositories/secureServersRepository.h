#ifndef SECURESERVERSREPOSITORY_H
#define SECURESERVERSREPOSITORY_H

#include <memory>
#include <QVector>
#include <QJsonArray>
#include <QJsonDocument>

#include "core/repositories/serversRepository.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "secure_qsettings.h"

using namespace amnezia;

class SecureServersRepository : public ServersRepository
{
public:
    explicit SecureServersRepository(SecureQSettings* settings);

    void addServer(const ServerConfig &server) override;
    void editServer(int index, const ServerConfig &server) override;
    void removeServer(int index) override;
    ServerConfig server(int index) const override;
    QVector<ServerConfig> servers() const override;
    int serversCount() const override;

    int defaultServerIndex() const override;
    void setDefaultServer(int index) override;

    void setDefaultContainer(int serverIndex, DockerContainer container) override;
    ContainerConfig containerConfig(int serverIndex, DockerContainer container) const override;
    void setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config) override;
    void clearLastConnectionConfig(int serverIndex, DockerContainer container) override;

    ServerCredentials serverCredentials(int index) const override;
    bool hasServerWithVpnKey(const QString &vpnKey) const override;

private:
    QJsonArray serversArray() const;
    void setServersArray(const QJsonArray &servers);
    
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    
    SecureQSettings* m_settings;
};

#endif // SECURESERVERSREPOSITORY_H

