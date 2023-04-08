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
    set_pageContentVisible(false);
    set_busyIndicatorIsRunning(true);

    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex);
    const DockerContainer container = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    const QString containerNameString = ContainerProps::containerHumanNames().value(container);
    set_labelCurrentVpnProtocolText(tr("Service: ") + containerNameString);

    const QVector<amnezia::Proto> protocols = ContainerProps::protocolsForContainer(container);
    if (!protocols.empty()) {
        const Proto currentMainProtocol = protocols.front();

        auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
        const QModelIndex modelIndex = model->index(m_currentClientIndex);

        set_lineEditNameAliasText(model->data(modelIndex, ClientManagementModel::ClientRoles::NameRole).toString());
        if (currentMainProtocol == Proto::OpenVpn) {
            const QString certId = model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertIdRole).toString();
            QString certData = model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertDataRole).toString();

            if (certData.isEmpty() && !certId.isEmpty()) {
                QString stdOut;
                auto cbReadStdOut = [&](const QString &data, libssh::Client &) {
                    stdOut += data + "\n";
                    return ErrorCode::NoError;
                };

                const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c 'cat /opt/amnezia/openvpn/pki/issued/%1.crt'")
                                                           .arg(certId);
                ServerController serverController(m_settings);
                const QString script = serverController.replaceVars(getOpenVpnCertData, serverController.genVarsForScript(credentials, container));
                ErrorCode error = serverController.runScript(credentials, script, cbReadStdOut);
                certData = stdOut;
                if (isErrorOccured(error)) {
                    set_busyIndicatorIsRunning(false);
                    emit uiLogic()->closePage();
                    return;
                }
            }
            set_labelOpenVpnCertId(certId);
            set_textAreaOpenVpnCertData(certData);
        } else if (currentMainProtocol == Proto::WireGuard) {
            set_textAreaWireGuardKeyData(model->data(modelIndex, ClientManagementModel::ClientRoles::WireGuardPublicKey).toString());
        }
    }
    set_pageContentVisible(true);
    set_busyIndicatorIsRunning(false);
}

void ClientInfoLogic::onLineEditNameAliasEditingFinished()
{
    set_busyIndicatorIsRunning(true);

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    const QModelIndex modelIndex = model->index(m_currentClientIndex);
    model->setData(modelIndex, m_lineEditNameAliasText, ClientManagementModel::ClientRoles::NameRole);

    const DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex);
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

    set_busyIndicatorIsRunning(false);
}

void ClientInfoLogic::onRevokeOpenVpnCertificateClicked()
{
    set_busyIndicatorIsRunning(true);
    const DockerContainer container = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex);

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    const QModelIndex modelIndex = model->index(m_currentClientIndex);
    const QString certId = model->data(modelIndex, ClientManagementModel::ClientRoles::OpenVpnCertIdRole).toString();

    const QString getOpenVpnCertData = QString("sudo docker exec -i $CONTAINER_NAME bash -c '"
                                               "cd /opt/amnezia/openvpn ;\\"
                                               "easyrsa revoke %1 ;\\"
                                               "easyrsa gen-crl ;\\"
                                               "cp pki/crl.pem .'").arg(certId);
    ServerController serverController(m_settings);
    const QString script = serverController.replaceVars(getOpenVpnCertData,
                                                        serverController.genVarsForScript(credentials, container));
    auto error = serverController.runScript(credentials, script);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        emit uiLogic()->goToPage(Page::ServerSettings);
        return;
    }

    model->removeRows(m_currentClientIndex);
    const QJsonObject clientsTable = model->getContent(Proto::OpenVpn);
    error = setClientsList(credentials, container, Proto::OpenVpn, clientsTable);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    const QJsonObject &containerConfig = m_settings->containerConfig(uiLogic()->m_selectedServerIndex, container);
    error = serverController.startupContainerWorker(credentials, container, containerConfig);
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    set_busyIndicatorIsRunning(false);
}

void ClientInfoLogic::onRevokeWireGuardKeyClicked()
{
    set_busyIndicatorIsRunning(true);
    ErrorCode error;
    const DockerContainer container = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    const ServerCredentials credentials = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex);

    ServerController serverController(m_settings);

    const QString wireGuardConfigFile = "opt/amnezia/wireguard/wg0.conf";
    const QString wireguardConfigString = serverController.getTextFileFromContainer(container, credentials, wireGuardConfigFile, &error);
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
    error = serverController.uploadTextFileToContainer(container, credentials, newWireGuardConfig,
                                                       protocols::wireguard::serverConfigPath,
                                                       libssh::SftpOverwriteMode::SftpOverwriteExisting);
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

    const QString script = "sudo docker exec -i $CONTAINER_NAME bash -c 'wg syncconf wg0 <(wg-quick strip /opt/amnezia/wireguard/wg0.conf)'";
    error = serverController.runScript(credentials,
                                       serverController.replaceVars(script, serverController.genVarsForScript(credentials, container)));
    if (isErrorOccured(error)) {
        set_busyIndicatorIsRunning(false);
        return;
    }

    set_busyIndicatorIsRunning(false);
}

ErrorCode ClientInfoLogic::setClientsList(const ServerCredentials &credentials, DockerContainer container, Proto mainProtocol, const QJsonObject &clietns)
{
    const QString mainProtocolString = ProtocolProps::protoToString(mainProtocol);
    const QString clientsTableFile = QString("opt/amnezia/%1/clientsTable").arg(mainProtocolString);
    ServerController serverController(m_settings);
    ErrorCode error = serverController.uploadTextFileToContainer(container, credentials, QJsonDocument(clietns).toJson(), clientsTableFile);
    return error;
}
