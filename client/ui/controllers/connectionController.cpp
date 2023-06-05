#include "connectionController.h"

#include <QApplication>

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           QObject *parent) : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel)
{

}

void ConnectionController::openConnection()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QModelIndex serverModelIndex = m_serversModel->index(serverIndex);
    ServerCredentials credentials = qvariant_cast<ServerCredentials>(m_serversModel->data(serverModelIndex,
                                                                                          ServersModel::ServersModelRoles::CredentialsRole));

    DockerContainer container = m_containersModel->getDefaultContainer();
    QModelIndex containerModelIndex = m_containersModel->index(container);
    const QJsonObject &containerConfig = qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex,
                                                                                            ContainersModel::Roles::ConfigRole));

//    if (m_settings->containers(serverIndex).isEmpty()) {
//        set_labelErrorText(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
//        set_pushButtonConnectChecked(false);
//        return;
//    }

//    if (container == DockerContainer::None) {
//        set_labelErrorText(tr("VPN Protocol not chosen"));
//        set_pushButtonConnectChecked(false);
//        return;
//    }

    //todo error handling
    qApp->processEvents();
    emit connectToVpn(serverIndex, credentials, container, containerConfig);
}

void ConnectionController::closeConnection()
{
    emit disconnectFromVpn();
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
