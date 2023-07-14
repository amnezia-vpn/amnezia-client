#include "ServerContainersLogic.h"
#include "ServerConfiguringProgressLogic.h"
#include "ShareConnectionLogic.h"

#include <QApplication>

#include "protocols/PageProtocolLogicBase.h"

#include "core/servercontroller.h"
#include <functional>

#include "../pages_logic/VpnLogic.h"
#include "../uilogic.h"
#include "core/errorstrings.h"
#include "vpnconnection.h"

ServerContainersLogic::ServerContainersLogic(UiLogic *logic, QObject *parent) : PageLogicBase(logic, parent)
{
}

void ServerContainersLogic::onUpdatePage()
{
    //    ContainersModel *c_model = qobject_cast<ContainersModel *>(uiLogic()->containersModel());
    //    c_model->setSelectedServerIndex(uiLogic()->m_selectedServerIndex);

    ProtocolsModel *p_model = qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel());
    //    p_model->setSelectedServerIndex(uiLogic()->m_selectedServerIndex);

    set_isManagedServer(m_settings->haveAuthData(uiLogic()->m_selectedServerIndex));
    uiLogic()->m_installCredentials = m_settings->serverCredentials(uiLogic()->m_selectedServerIndex);
    emit updatePage();
}

void ServerContainersLogic::onPushButtonProtoSettingsClicked(DockerContainer c, Proto p)
{
    qDebug() << "ServerContainersLogic::onPushButtonProtoSettingsClicked" << c << p;
    uiLogic()->m_selectedDockerContainer = c;
    uiLogic()->protocolLogic(p)->updateProtocolPage(
            m_settings->protocolConfig(uiLogic()->m_selectedServerIndex, uiLogic()->m_selectedDockerContainer, p),
            uiLogic()->m_selectedDockerContainer, m_settings->haveAuthData(uiLogic()->m_selectedServerIndex));

    emit uiLogic()->goToProtocolPage(p);
}

void ServerContainersLogic::onPushButtonDefaultClicked(DockerContainer c)
{
    if (m_settings->defaultContainer(uiLogic()->m_selectedServerIndex) == c)
        return;

    m_settings->setDefaultContainer(uiLogic()->m_selectedServerIndex, c);
    uiLogic()->onUpdateAllPages();

    if (uiLogic()->m_selectedServerIndex != m_settings->defaultServerIndex())
        return;
    if (!uiLogic()->m_vpnConnection)
        return;
    if (!uiLogic()->m_vpnConnection->isConnected())
        return;

    uiLogic()->pageLogic<VpnLogic>()->onDisconnect();
    uiLogic()->pageLogic<VpnLogic>()->onConnect();
}

void ServerContainersLogic::onPushButtonShareClicked(DockerContainer c)
{
    uiLogic()->pageLogic<ShareConnectionLogic>()->updateSharingPage(uiLogic()->m_selectedServerIndex, c);
    emit uiLogic()->goToPage(Page::ShareConnection);
}

void ServerContainersLogic::onPushButtonRemoveClicked(DockerContainer container)
{
    // buttonSetEnabledFunc(false);
    ServerController serverController(m_settings);
    ErrorCode e =
            serverController.removeContainer(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex), container);
    m_settings->removeContainerConfig(uiLogic()->m_selectedServerIndex, container);
    // buttonSetEnabledFunc(true);

    if (m_settings->defaultContainer(uiLogic()->m_selectedServerIndex) == container) {
        const auto &c = m_settings->containers(uiLogic()->m_selectedServerIndex);
        if (c.isEmpty())
            m_settings->setDefaultContainer(uiLogic()->m_selectedServerIndex, DockerContainer::None);
        else
            m_settings->setDefaultContainer(uiLogic()->m_selectedServerIndex, c.keys().first());
    }
    uiLogic()->onUpdateAllPages();
}

void ServerContainersLogic::onPushButtonContinueClicked(DockerContainer c, int port, TransportProto tp)
{
    ServerController serverController(m_settings);
    QJsonObject config; // = serverController.createContainerInitialConfig(c, port, tp);

    emit uiLogic()->goToPage(Page::ServerConfiguringProgress);
    qApp->processEvents();

    bool isServerCreated = false;
    ErrorCode errorCode = uiLogic()->addAlreadyInstalledContainersGui(isServerCreated);

    if (errorCode == ErrorCode::NoError) {
        if (!uiLogic()->isContainerAlreadyAddedToGui(c)) {
            auto installAction = [this, c, &config]() {
                ServerController serverController(m_settings);
                return serverController.setupContainer(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex),
                                                       c, config);
            };
            errorCode = uiLogic()->pageLogic<ServerConfiguringProgressLogic>()->doInstallAction(installAction);

            if (errorCode == ErrorCode::NoError) {
                m_settings->setContainerConfig(uiLogic()->m_selectedServerIndex, c, config);
                if (ContainerProps::containerService(c) == ServiceType::Vpn) {
                    m_settings->setDefaultContainer(uiLogic()->m_selectedServerIndex, c);
                }
            }
        } else {
            emit uiLogic()->showWarningMessage(
                    "Attention! The container you are trying to install is already installed on the server. "
                    "All installed containers have been added to the application ");
        }

        uiLogic()->onUpdateAllPages();
    }
    if (errorCode != ErrorCode::NoError) {
        emit uiLogic()->showWarningMessage(tr("Error occurred while configuring server.") + "\n" + tr("Error message: ")
                                           + errorString(errorCode) + "\n" + tr("See logs for details."));
    }
    emit uiLogic()->closePage();
}
