#include "ClientInfoLogic.h"

#include "core/servercontroller.h"
#include "ui/models/clientManagementModel.h"
#include "ui/uilogic.h"

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
    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);

    auto protocols = ContainerProps::protocolsForContainer(selectedContainer);
    if (!protocols.empty()) {
        auto currentMainProtocol = protocols.front();

        auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
        auto modelIndex = model->index(m_currentClientIndex);

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
    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    auto modelIndex = model->index(m_currentClientIndex);
    model->setData(modelIndex, m_lineEditNameAliasText, ClientManagementModel::ClientRoles::NameRole);

    auto clientsTable = model->getContent();
    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    auto protocols = ContainerProps::protocolsForContainer(selectedContainer);
    if (!protocols.empty()) {
        auto currentMainProtocol = protocols.front();
        m_serverController->setClientsList(m_settings->serverCredentials(uiLogic()->selectedServerIndex),
                                           selectedContainer,
                                           currentMainProtocol,
                                           clientsTable);
    }
}

void ClientInfoLogic::onRevokeOpenVpnCertificateClicked()
{

}

void ClientInfoLogic::onRevokeWireGuardKeyClicked()
{

}
