#ifndef SECURESERVERSREPOSITORY_H
#define SECURESERVERSREPOSITORY_H

#include <memory>
#include <QJsonObject>
#include <QJsonArray>

#include "core/repositories/serversRepository.h"
#include "settings.h"

using namespace amnezia;

class SecureServersRepository : public ServersRepository
{
public:
    explicit SecureServersRepository(std::shared_ptr<Settings> settings);

    void addServer(const QJsonObject &server) override;
    void editServer(int index, const QJsonObject &server) override;
    void removeServer(int index) override;
    QJsonObject server(int index) const override;
    QJsonArray serversArray() const override;
    int serversCount() const override;

    int defaultServerIndex() const override;
    void setDefaultServer(int index) override;

    void setDefaultContainer(int serverIndex, DockerContainer container) override;
    QJsonObject containerConfig(int serverIndex, DockerContainer container) const override;
    void setContainerConfig(int serverIndex, DockerContainer container, const QJsonObject &config) override;
    void clearLastConnectionConfig(int serverIndex, DockerContainer container) override;

    ServerCredentials serverCredentials(int index) const override;

private:
    std::shared_ptr<Settings> m_settings;
};

#endif // SECURESERVERSREPOSITORY_H

