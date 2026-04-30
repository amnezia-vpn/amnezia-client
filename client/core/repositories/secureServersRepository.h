#ifndef SECURESERVERSREPOSITORY_H
#define SECURESERVERSREPOSITORY_H

#include <QObject>
#include <QVector>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtGlobal>

#include "core/models/serverConfig.h"
#include "core/models/containerConfig.h"
#include "secureQSettings.h"

using namespace amnezia;

class SecureServersRepository : public QObject
{
    Q_OBJECT

public:
    explicit SecureServersRepository(SecureQSettings* settings, QObject *parent = nullptr);

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
    bool hasServerWithCrc(quint16 crc) const;

    void setServersArray(const QJsonArray &servers);

    void invalidateCache();

signals:
    void serverAdded(ServerConfig config);
    void serverEdited(int index, ServerConfig config);
    void serverRemoved(int index);
    void defaultServerChanged(int index);

private:
    void syncToStorage();
    QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &key, const QVariant &value);
    
    SecureQSettings* m_settings;
    
    QVector<ServerConfig> m_servers;
    int m_defaultServerIndex = 0;
};

#endif // SECURESERVERSREPOSITORY_H

