#include "connectionController.h"

#include <QApplication>

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           QObject *parent) : QObject(parent), m_serversModel(serversModel), m_containersModel(containersModel)
{

}

bool ConnectionController::onConnectionButtonClicked()
{
    if (!isConnected) {
        openVpnConnection();
    } else {
        closeVpnConnection();
    }
}

bool ConnectionController::openVpnConnection()
{
    int serverIndex = m_serversModel->getDefaultServerIndex();
    QModelIndex serverModelIndex = m_serversModel->index(serverIndex);
    ServerCredentials credentials = qvariant_cast<ServerCredentials>(m_serversModel->data(serverModelIndex,
                                                                                          ServersModel::ServersModelRoles::CredentialsRole));

    DockerContainer container = m_containersModel->getDefaultContainer();
    QModelIndex containerModelIndex = m_containersModel->index(container);
    const QJsonObject &containerConfig = qvariant_cast<QJsonObject>(m_containersModel->data(containerModelIndex,
                                                                                            ContainersModel::ContainersModelRoles::ConfigRole));

    //todo error handling
    qApp->processEvents();
    emit connectToVpn(serverIndex, credentials, container, containerConfig);
    isConnected = true;


//    int serverIndex = m_settings->defaultServerIndex();
//    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);
//    DockerContainer container = m_settings->defaultContainer(serverIndex);

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


//    const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);

//    set_labelErrorText("");
//    set_pushButtonConnectChecked(true);
//    set_pushButtonConnectEnabled(false);

//    qApp->processEvents();

//    emit connectToVpn(serverIndex, credentials, container, containerConfig);
}

bool ConnectionController::closeVpnConnection()
{
    emit disconnectFromVpn();
}

