#ifndef QSERVERSREPOSITORY_H
#define QSERVERSREPOSITORY_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>

#include "core/repositories/serversRepository.h"
#include "settings.h"

using namespace amnezia;

class SecureServersRepository;

class QServersRepository : public QObject
{
    Q_OBJECT

public:
    explicit QServersRepository(std::shared_ptr<Settings> settings, QObject *parent = nullptr);
    ~QServersRepository();

    ServersRepository* repository();

    void addServer(const QJsonObject &server);
    void editServer(int index, const QJsonObject &server);
    void removeServer(int index);
    QJsonObject server(int index) const;
    QJsonArray serversArray() const;
    int serversCount() const;

    int defaultServerIndex() const;
    void setDefaultServer(int index);

    void setDefaultContainer(int serverIndex, DockerContainer container);
    QJsonObject containerConfig(int serverIndex, DockerContainer container) const;
    void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config);
    void clearLastConnectionConfig(int serverIndex, DockerContainer container);

    ServerCredentials serverCredentials(int index) const;
    bool hasServerWithVpnKey(const QString &vpnKey) const;

signals:
    void serverAdded(QJsonObject config);
    void serverEdited(int index, QJsonObject config);
    void serverRemoved(int index);
    void defaultServerChanged(int index);

private:
    std::unique_ptr<SecureServersRepository> m_secureRepository;
};

#endif // QSERVERSREPOSITORY_H

