#include "connectionController.h"

#include <QApplication>

#include "core/errorstrings.h"

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection,
                                           QObject *parent)
    : QObject(parent)
    , m_serversModel(serversModel)
    , m_containersModel(containersModel)
    , m_vpnConnection(vpnConnection)
{
    connect(m_vpnConnection.get(),
            &VpnConnection::connectionStateChanged,
            this,
            &ConnectionController::connectionStateChanged);
    connect(this,
            &ConnectionController::connectToVpn,
            m_vpnConnection.get(),
            &VpnConnection::connectToVpn,
            Qt::QueuedConnection);
    connect(this,
            &ConnectionController::disconnectFromVpn,
            m_vpnConnection.get(),
            &VpnConnection::disconnectFromVpn,
            Qt::QueuedConnection);
}

void ConnectionController::openConnection()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    ServerCredentials credentials = qvariant_cast<ServerCredentials>(
        m_serversModel->data(serverIndex, ServersModel::ServersModelRoles::CredentialsRole));

    DockerContainer container = m_containersModel->getDefaultContainer();
    QModelIndex containerModelIndex = m_containersModel->index(container);
    const QJsonObject &containerConfig = qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex,
                                                                                            ContainersModel::Roles::ConfigRole));

    if (container == DockerContainer::None) {
        emit connectionErrorOccurred(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        return;
    }

    qApp->processEvents();
    emit connectToVpn(serverIndex, credentials, container, containerConfig);
}

void ConnectionController::closeConnection()
{
    emit disconnectFromVpn();
}

QString ConnectionController::getLastConnectionError()
{
    return errorString(m_vpnConnection->lastError());
}

bool ConnectionController::isConnected()
{
    return m_isConnected;
}

void ConnectionController::setIsConnected(bool isConnected)
{
    m_isConnected = isConnected;
    emit isConnectedChanged();
}
