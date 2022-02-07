#include "ServerContainersLogic.h"
#include "ShareConnectionLogic.h"
#include "ServerConfiguringProgressLogic.h"

#include <QApplication>

#include "protocols/CloakLogic.h"
#include "protocols/OpenVpnLogic.h"
#include "protocols/ShadowSocksLogic.h"

#include "core/servercontroller.h"
#include <functional>

#include "../uilogic.h"
#include "../pages_logic/VpnLogic.h"
#include "vpnconnection.h"


ServerContainersLogic::ServerContainersLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{
}

void ServerContainersLogic::onUpdatePage()
{
    ContainersModel *c_model = qobject_cast<ContainersModel *>(uiLogic()->containersModel());
    c_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);

    ProtocolsModel *p_model = qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel());
    p_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);

    emit updatePage();
}

void ServerContainersLogic::onPushButtonProtoSettingsClicked(DockerContainer c, Proto p)
{
    qDebug()<< "ServerContainersLogic::onPushButtonProtoSettingsClicked" << c << p;
    uiLogic()->selectedDockerContainer = c;
    uiLogic()->protocolLogic(p)->updateProtocolPage(m_settings.protocolConfig(uiLogic()->selectedServerIndex, uiLogic()->selectedDockerContainer, p),
                      uiLogic()->selectedDockerContainer,
                      m_settings.haveAuthData(uiLogic()->selectedServerIndex));

    emit uiLogic()->goToProtocolPage(p);
}

void ServerContainersLogic::onPushButtonDefaultClicked(DockerContainer c)
{
    if (m_settings.defaultContainer(uiLogic()->selectedServerIndex) == c) return;

    m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c);
    uiLogic()->onUpdateAllPages();

    if (uiLogic()->selectedServerIndex != m_settings.defaultServerIndex()) return;
    if (!uiLogic()->m_vpnConnection) return;
    if (!uiLogic()->m_vpnConnection->isConnected()) return;

    uiLogic()->vpnLogic()->onDisconnect();
    uiLogic()->vpnLogic()->onConnect();
}

void ServerContainersLogic::onPushButtonShareClicked(DockerContainer c)
{
    uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, c);
    emit uiLogic()->goToPage(Page::ShareConnection);
}

void ServerContainersLogic::onPushButtonRemoveClicked(DockerContainer container)
{
    //buttonSetEnabledFunc(false);
    ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), container);
    m_settings.removeContainerConfig(uiLogic()->selectedServerIndex, container);
    //buttonSetEnabledFunc(true);

    if (m_settings.defaultContainer(uiLogic()->selectedServerIndex) == container) {
        const auto &c = m_settings.containers(uiLogic()->selectedServerIndex);
        if (c.isEmpty()) m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, DockerContainer::None);
        else m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c.keys().first());
    }
    uiLogic()->onUpdateAllPages();
}

void ServerContainersLogic::onPushButtonContinueClicked(DockerContainer c, int port, TransportProto tp)
{
    QJsonObject config = ServerController::createContainerInitialConfig(c, port, tp);

    emit uiLogic()->goToPage(Page::ServerConfiguringProgress);
    qApp->processEvents();

    ErrorCode e = uiLogic()->serverConfiguringProgressLogic()->doInstallAction([this, c, &config](){
        return ServerController::setupContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), c, config);
    });

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, c, config);
        if (ContainerProps::containerService(c) == ServiceType::Vpn) {
            m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c);
        }
    }

    uiLogic()->onUpdateAllPages();
    emit uiLogic()->closePage();
}
