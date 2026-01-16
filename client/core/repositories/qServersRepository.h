#ifndef QSERVERSREPOSITORY_H
#define QSERVERSREPOSITORY_H

#include <QObject>
#include <QVector>
#include <memory>

#include "core/repositories/serversRepository.h"
#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "secure_qsettings.h"

using namespace amnezia;

class SecureServersRepository;

class QServersRepository : public QObject
{
    Q_OBJECT

public:
    explicit QServersRepository(SecureQSettings* settings, QObject *parent = nullptr);
    ~QServersRepository();

    ServersRepository* repository();

    void addServer(const ServerConfig &server);
    void editServer(int index, const ServerConfig &server);
    void removeServer(int index);
    ServerConfig server(int index) const;
    QVector<ServerConfig> servers() const;
    int serversCount() const;

    int defaultServerIndex() const;
    void setDefaultServer(int index);

    void setDefaultContainer(int serverIndex, DockerContainer container);
    ContainerConfig containerConfig(int serverIndex, DockerContainer container) const;
    void setContainerConfig(int serverIndex, DockerContainer container, const ContainerConfig &config);
    void clearLastConnectionConfig(int serverIndex, DockerContainer container);

    ServerCredentials serverCredentials(int index) const;
    bool hasServerWithVpnKey(const QString &vpnKey) const;

signals:
    void serverAdded(ServerConfig config);
    void serverEdited(int index, ServerConfig config);
    void serverRemoved(int index);
    void defaultServerChanged(int index);

private:
    std::unique_ptr<SecureServersRepository> m_secureRepository;
};

#endif // QSERVERSREPOSITORY_H

