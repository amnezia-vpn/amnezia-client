#ifndef CONNECTIONCONTROLLER_H
#define CONNECTIONCONTROLLER_H

#include <QJsonObject>
#include <QPair>
#include <memory>

#include "containers/containers_defs.h"
#include "core/utils/defs.h"
#include "core/repositories/qServersRepository.h"
#include "core/repositories/qAppSettingsRepository.h"
#include "vpnconnection.h"

using namespace amnezia;

class ConnectionController
{
public:
    explicit ConnectionController(QServersRepository* serversRepository,
                                 QAppSettingsRepository* appSettingsRepository,
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
                                             const ServerConfig &serverConfig,
                                             const ContainerConfig &containerConfig,
                                             DockerContainer container);

    bool isServiceReady() const;

    bool isContainerSupported(DockerContainer container) const;

private:
    QServersRepository* m_serversRepository;
    QAppSettingsRepository* m_appSettingsRepository;
    VpnConnection* m_vpnConnection;
};

#endif
