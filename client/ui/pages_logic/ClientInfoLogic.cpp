#include "ClientInfoLogic.h"

#include <QMessageBox>

#include "defines.h"
#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include "ui/models/clientManagementModel.h"
#include "ui/uilogic.h"

namespace {
    bool isErrorOccured(ErrorCode error) {
        if (error != ErrorCode::NoError) {
            QMessageBox::warning(nullptr, APPLICATION_NAME,
                                 QObject::tr("An error occurred while saving the list of clients.") + "\n" + errorString(error));
            return true;
        }
        return false;
    }
}

ClientInfoLogic::ClientInfoLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void ClientInfoLogic::setCurrentClientId(int index)
{
    m_currentClientIndex = index;
}

void ClientInfoLogic::onUpdatePage()
{
    set_busyIndicatorIsRunning(false);

    const DockerContainer container = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    const QString containerNameString = ContainerProps::containerHumanNames().value(container);
    set_labelCurrentVpnProtocolText(tr("Service: ") + containerNameString);

    const QVector<amnezia::Proto> protocols = ContainerProps::protocolsForContainer(container);
    if (!protocols.empty()) {
        const Proto currentMainProtocol = protocols.front();

        auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
        const QModelIndex modelIndex = model->index(m_currentClientIndex);

        set_lineEditNameAliasText(model->data(modelIndex, ClientManagementModel::ClientRoles::NameRole).toString());
        if (currentMainProtocol == Proto::OpenVpn) {
            set_labelOpenVpnCertId(model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertIdRole).toString());
            set_textAreaOpenVpnCertData(model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertDataRole).toString());
        } else if (currentMainProtocol == Proto::WireGuard) {
            set_textAreaWireGuardKeyData(model->data(modelIndex, ClientManagementModel::ClientRoles::WireGuardPublicKey).toString());
        }
    }
}

void ClientInfoLogic::onLineEditNameAliasEditingFinished()
{
    set_busyIndicatorIsRunning(true);

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    const QModelIndex modelIndex = model->index(m_currentClientIndex);
    model->setData(modelIndex, m_lineEditNameAliasText, ClientManagementModel::ClientRoles::NameRole);

    const DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->selectedServerIndex);
    const QVector<amnezia::Proto> protocols = ContainerProps::protocolsForContainer(selectedContainer);
    if (!protocols.empty()) {
        const Proto currentMainProtocol = protocols.front();
        const QJsonObject clientsTable = model->getContent(currentMainProtocol);
        ErrorCode error = setClientsList(credentials,
                                         selectedContainer,
                                         currentMainProtocol,
                                         clientsTable);
        isErrorOccured(error);
    }

    m_serverController->disconnectFromHost(credentials);
    set_busyIndicatorIsRunning(false);
}

void ClientInfoLogic::onRevokeOpenVpnCertificateClicked()
{
    set_busyIndicatorIsRunning(true);
    const DockerContainer container = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->selectedServerIndex);

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    const QModelIndex modelIndex = model->index(m_currentClientIndex);
    const QString certId = model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertIdRole).toString();

    const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c '"
                                               "cd /opt/amnezia/openvpn ;\\"
                                               "easyrsa revoke %1 ;\\"
                                               "easyrsa gen-crl ;\\"
                                               "cp pki/crl.pem .'").arg(certId);
    const QString script = m_serverController->replaceVars(getOpenVpnCertData,
                                                           m_serverController->genVarsForScript(credentials, container));
    auto error = m_serverController->runScript(credentials, script);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    model->removeRows(m_currentClientIndex);
    const QJsonObject clientsTable = model->getContent(Proto::OpenVpn);
    error = setClientsList(credentials, container, Proto::OpenVpn, clientsTable);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    error = m_serverController->uploadTextFileToContainer(container, credentials, "crl-verify crl.pem\n",
                                                          protocols::openvpn::serverConfigPath,
                                                          QSsh::SftpOverwriteMode::SftpAppendToExisting);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    const QJsonObject &containerConfig = m_settings->containerConfig(uiLogic()->selectedServerIndex, container);
    error = m_serverController->startupContainerWorker(credentials, container, containerConfig);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }
    m_serverController->disconnectFromHost(credentials);
    set_busyIndicatorIsRunning(false);
}

void ClientInfoLogic::onRevokeWireGuardKeyClicked()
{
    set_busyIndicatorIsRunning(true);
    ErrorCode error;
    const DockerContainer container = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->selectedServerIndex);
    const QString wireGuardConfigFile = "opt/amnezia/wireguard/wg0.conf";
    const QString wireguardConfigString = m_serverController->getTextFileFromContainer(container, credentials, wireGuardConfigFile, &error);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    const QModelIndex modelIndex = model->index(m_currentClientIndex);
    const QString key = model->data(modelIndex, ClientManagementModel::ClientRoles::WireGuardPublicKey).toString();

    auto configSections = wireguardConfigString.split("[", Qt::SkipEmptyParts);
    for (auto &section : configSections) {
        if (section.contains(key)) {
            configSections.removeOne(section);
        }
    }
    QString newWireGuardConfig = configSections.join("[");
    newWireGuardConfig.insert(0, "[");
    error = m_serverController->uploadTextFileToContainer(container, credentials, newWireGuardConfig,
                                                          protocols::wireguard::serverConfigPath,
                                                          QSsh::SftpOverwriteMode::SftpOverwriteExisting);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    model->removeRows(m_currentClientIndex);
    const QJsonObject clientsTable = model->getContent(Proto::WireGuard);
    error = setClientsList(credentials, container, Proto::WireGuard, clientsTable);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    error = m_serverController->runScript(credentials,
                                          m_serverController->replaceVars("sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip /opt/amnezia/wireguard/wg0.conf)'",
                                                                          m_serverController->genVarsForScript(credentials, container)));
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }
    m_serverController->disconnectFromHost(credentials);
    set_busyIndicatorIsRunning(false);
}

ErrorCode ClientInfoLogic::setClientsList(const ServerCredentials &credentials, DockerContainer container, Proto mainProtocol, const QJsonObject &clietns)
{
    const QString mainProtocolString = ProtocolProps::protoToString(mainProtocol);
    const QString clientsTableFile = QString("opt/amnezia/%1/clientsTable").arg(mainProtocolString);
    ErrorCode error = m_serverController->uploadTextFileToContainer(container, credentials, QJsonDocument(clietns).toJson(), clientsTableFile);
    return error;
}
