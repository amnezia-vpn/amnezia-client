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

ServerContainersLogic::ServerContainersLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent)
{
}

void ServerContainersLogic::updateServerContainersPage()
{
    ContainersModel *c_model = qobject_cast<ContainersModel *>(uiLogic()->containersModel());
    c_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);

    ProtocolsModel *p_model = qobject_cast<ProtocolsModel *>(uiLogic()->protocolsModel());
    p_model->setSelectedServerIndex(uiLogic()->selectedServerIndex);
}

void ServerContainersLogic::onPushButtonProtoSettingsClicked(DockerContainer c, Protocol p)
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
    m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c);
    updateServerContainersPage();
}

void ServerContainersLogic::onPushButtonShareClicked(DockerContainer c)
{
    uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), c);
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
    updateServerContainersPage();
}

void ServerContainersLogic::onPushButtonContinueClicked(DockerContainer c, int port, TransportProto tp)
{
    QMap<DockerContainer, QJsonObject> containers;
    Protocol mainProto = ContainerProps::defaultProtocol(c);

    QJsonObject config {
        { config_key::container, ContainerProps::containerToString(c) },
        { ProtocolProps::protoToString(mainProto), QJsonObject {
                { config_key::port, QString::number(port) },
                { config_key::transport_proto, ProtocolProps::transportProtoToString(tp, mainProto) }}
        }
    };

    containers.insert(c, config);

    emit uiLogic()->goToPage(Page::ServerConfiguringProgress);
    qApp->processEvents();

    ErrorCode e = uiLogic()->serverConfiguringProgressLogic()->doInstallAction([this, c](){
        return ServerController::setupContainer(m_settings.serverCredentials(uiLogic()->selectedServerIndex), c);
    });

    if (!e) {
        m_settings.setContainerConfig(uiLogic()->selectedServerIndex, c, QJsonObject());
        m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, c);
    }

    updateServerContainersPage();
    emit uiLogic()->closePage();
}
