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

    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    auto modelIndex = model->index(m_currentClientIndex);
    set_lineEditNameAliasText(model->data(modelIndex, ClientManagementModel::ClientRoles::NameRole).toString());
    set_labelCertId(model->data(modelIndex, ClientManagementModel::ClientRoles::CertIdRole).toString());
    set_textAreaCertificate(model->data(modelIndex, ClientManagementModel::ClientRoles::CertDataRole).toString());
}

void ClientInfoLogic::onLineEditNameAliasEditingFinished()
{    
    auto model = qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel());
    auto modelIndex = model->index(m_currentClientIndex);
    model->setData(modelIndex, m_lineEditNameAliasText, ClientManagementModel::ClientRoles::NameRole);

    m_serverController->setClientsList();
}

void ClientInfoLogic::onRevokeCertificateClicked()
{

}
