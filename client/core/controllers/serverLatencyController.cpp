#include "serverLatencyController.h"

#include <QJsonDocument>

#include "core/controllers/connectionController.h"
#include "core/controllers/serversController.h"
#include "core/controllers/settingsController.h"
#include "core/protocols/protocolUtils.h"

using namespace amnezia;

ServerLatencyController::ServerLatencyController(ServersController *serversController,
                                                 SettingsController *settingsController,
                                                 ConnectionController *connectionController, QObject *parent)
    : QObject(parent),
      m_serversController(serversController),
      m_settingsController(settingsController),
      m_connectionController(connectionController)
{
}

void ServerLatencyController::testAllServers()
{
    if (m_connectionController->isConnected()) {
        testDefaultServer();
        return;
    }

    for (const ServerDescription &server :
         m_serversController->buildServerDescriptions(m_settingsController->isAmneziaDnsEnabled())) {
        startProbe(server);
    }
}

void ServerLatencyController::testDefaultServer()
{
    const QString defaultServerId = m_serversController->getDefaultServerId();
    for (const ServerDescription &server :
         m_serversController->buildServerDescriptions(m_settingsController->isAmneziaDnsEnabled())) {
        if (server.serverId == defaultServerId) {
            startProbe(server);
            return;
        }
    }
}

bool ServerLatencyController::isTesting() const
{
    return !m_activeProbes.isEmpty();
}

bool ServerLatencyController::isTesting(const QString &serverId) const
{
    return m_activeProbes.contains(serverId);
}

std::optional<LatencyResult> ServerLatencyController::result(const QString &serverId) const
{
    const auto it = m_results.constFind(serverId);
    if (it == m_results.constEnd()) {
        return std::nullopt;
    }
    return it.value();
}

void ServerLatencyController::startProbe(const ServerDescription &server)
{
    if (m_activeProbes.contains(server.serverId)) {
        return;
    }

    const std::optional<LatencyEndpoint> endpoint = endpointFor(server);
    if (!endpoint.has_value()) {
        return;
    }

    const bool wasTesting = isTesting();
    auto *probe = new LatencyProbe(endpoint.value(), this);
    m_activeProbes.insert(server.serverId, probe);

    connect(probe, &LatencyProbe::finished, this, [this, probe, serverId = server.serverId](const LatencyResult &result) {
        m_activeProbes.remove(serverId);
        probe->deleteLater();
        m_results.insert(serverId, result);
        emit serverLatencyChanged(serverId);
        if (!isTesting()) {
            emit testingChanged();
        }
    });

    if (!wasTesting) {
        emit testingChanged();
    }
    emit serverLatencyChanged(server.serverId);
    probe->start();
}

std::optional<LatencyEndpoint> ServerLatencyController::endpointFor(const ServerDescription &server) const
{
    if (server.hostName.isEmpty()) {
        return std::nullopt;
    }

    const ContainerConfig containerConfig =
            m_serversController->getContainerConfig(server.serverId, server.defaultContainer);

    QString port = containerConfig.protocolConfig.port();
    QString transportProto = containerConfig.protocolConfig.transportProto();

    const OpenVpnProtocolConfig *openVpnConfig = containerConfig.getOpenVpnProtocolConfig();
    if (port.isEmpty() && openVpnConfig && openVpnConfig->hasClientConfig()) {
        port = openVpnConfig->clientConfig->remotePort();
        transportProto = openVpnConfig->clientConfig->remoteTransportProto();
    }

    const XrayProtocolConfig *xrayConfig = containerConfig.getXrayProtocolConfig();
    if (port.isEmpty() && xrayConfig && xrayConfig->hasClientConfig()) {
        const QJsonObject nativeConfig =
                QJsonDocument::fromJson(xrayConfig->clientConfig->nativeConfig.toUtf8()).object();
        port = XrayClientConfig::serverPortFromNativeJson(nativeConfig);
    }

    const TransportProto transport = transportProto.isEmpty()
            ? ProtocolUtils::defaultTransportProto(containerConfig.getProtocolType())
            : ProtocolUtils::transportProtoFromString(transportProto);

    return LatencyEndpoint { server.hostName, port.toUShort(), transport == TransportProto::Tcp };
}
