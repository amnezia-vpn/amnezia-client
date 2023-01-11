#include "ClientManagementLogic.h"

#include <QMessageBox>

#include "defines.h"
#include "core/errorstrings.h"
#include "core/servercontroller.h"
#include "ui/pages_logic/ClientInfoLogic.h"
#include "ui/models/clientManagementModel.h"
#include "ui/uilogic.h"

ClientManagementLogic::ClientManagementLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{

}

void ClientManagementLogic::onUpdatePage()
{
    qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel())->clearData();
    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);

    QJsonObject clients;

    auto protocols = ContainerProps::protocolsForContainer(selectedContainer);
    if (!protocols.empty()) {
        m_currentMainProtocol = protocols.front();

        ErrorCode error = m_serverController->getClientsList(m_settings->serverCredentials(uiLogic()->selectedServerIndex),
                                                          selectedContainer, m_currentMainProtocol, clients);
        if (error != ErrorCode::NoError) {
            QMessageBox::warning(nullptr, APPLICATION_NAME,
                                 tr("An error occurred while getting the list of clients.") + "\n" + errorString(error));
            return;
        }
    }
    QVector<QVariant> clientsArray;
    for (auto &clientId : clients.keys()) {
        clientsArray.push_back(clients[clientId].toObject());
    }
    qobject_cast<ClientManagementModel*>(uiLogic()->clientManagementModel())->setContent(clientsArray);
}

void ClientManagementLogic::onClientItemClicked(int index)
{
    uiLogic()->pageLogic<ClientInfoLogic>()->setCurrentClientId(index);
    emit uiLogic()->goToClientInfoPage(m_currentMainProtocol);
}
