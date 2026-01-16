#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QJsonObject>
#include <QPair>
#include <memory>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/repositories/serversRepository.h"
#include "core/repositories/appSettingsRepository.h"
#include "vpnconnection.h"

using namespace amnezia;

class ConnectionController
{
public:
    explicit ConnectionController(ServersRepository* serversRepository,
                                 AppSettingsRepository* appSettingsRepository,
                                 VpnConnection* vpnConnection);
    ~ConnectionController() = default;

    ErrorCode prepareConnection(int serverIndex,
                               QJsonObject& vpnConfiguration,
                               ServerCredentials& credentials,
                               DockerContainer& container);

    ErrorCode connectToVpn(int serverIndex);

    void disconnectFromVpn();

    ErrorCode lastError() const;

    QJsonObject createConnectionConfiguration(const QPair<QString, QString> &dns,
                                             const QJsonObject &serverConfig,
                                             const QJsonObject &containerConfig,
                                             DockerContainer container);

    bool isServiceReady() const;

    bool isContainerSupported(DockerContainer container) const;

private:
    QPair<QString, QString> getDnsPair(int serverIndex, bool isAmneziaDnsEnabled) const;

    ServersRepository* m_serversRepository;
    AppSettingsRepository* m_appSettingsRepository;
    VpnConnection* m_vpnConnection;
};

#endif
