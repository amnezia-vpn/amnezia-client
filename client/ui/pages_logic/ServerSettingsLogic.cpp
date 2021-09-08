#include "ServerSettingsLogic.h"
#include "vpnconnection.h"

#include "../uilogic.h"
#include "ServerListLogic.h"
#include "ShareConnectionLogic.h"
#include "VpnLogic.h"

#include "core/errorstrings.h"
#include <core/servercontroller.h>

ServerSettingsLogic::ServerSettingsLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_labelWaitInfoVisible{true},
    m_pushButtonClearVisible{true},
    m_pushButtonClearClientCacheVisible{true},
    m_pushButtonShareFullVisible{true},
    m_pushButtonClearText{tr("Clear server from Amnezia software")},
    m_pushButtonClearClientCacheText{tr("Clear client cached profile")}
{

}

void ServerSettingsLogic::updatePage()
{
    set_labelWaitInfoVisible(false);
    set_labelWaitInfoText("");
    set_pushButtonClearVisible(m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    set_pushButtonClearClientCacheVisible(m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    set_pushButtonShareFullVisible(m_settings.haveAuthData(uiLogic()->selectedServerIndex));
    QJsonObject server = m_settings.server(uiLogic()->selectedServerIndex);
    QString port = server.value(config_key::port).toString();
    set_labelServerText(QString("%1@%2%3%4")
                                     .arg(server.value(config_key::userName).toString())
                                     .arg(server.value(config_key::hostName).toString())
                                     .arg(port.isEmpty() ? "" : ":")
                                     .arg(port));
    set_lineEditDescriptionText(server.value(config_key::description).toString());
    QString selectedContainerName = m_settings.defaultContainerName(uiLogic()->selectedServerIndex);
    set_labelCurrentVpnProtocolText(tr("Protocol: ") + selectedContainerName);
}

void ServerSettingsLogic::onPushButtonClearServer()
{
    set_pageEnabled(false);
    set_pushButtonClearText(tr("Uninstalling Amnezia software..."));

    if (m_settings.defaultServerIndex() == uiLogic()->selectedServerIndex) {
        uiLogic()->vpnLogic()->onDisconnect();
    }

    ErrorCode e = ServerController::removeAllContainers(m_settings.serverCredentials(uiLogic()->selectedServerIndex));
    ServerController::disconnectFromHost(m_settings.serverCredentials(uiLogic()->selectedServerIndex));
    if (e) {
        uiLogic()->setDialogConnectErrorText(
                    tr("Error occurred while configuring server.") + "\n" +
                    errorString(e) + "\n" +
                    tr("See logs for details."));
        emit uiLogic()->showConnectErrorDialog();
    }
    else {
        set_labelWaitInfoVisible(true);
        set_labelWaitInfoText(tr("Amnezia server successfully uninstalled"));
    }

    m_settings.setContainers(uiLogic()->selectedServerIndex, {});
    m_settings.setDefaultContainer(uiLogic()->selectedServerIndex, DockerContainer::None);

    set_pageEnabled(true);
    set_pushButtonClearText(tr("Clear server from Amnezia software"));
}

void ServerSettingsLogic::onPushButtonForgetServer()
{
    if (m_settings.defaultServerIndex() == uiLogic()->selectedServerIndex && uiLogic()->m_vpnConnection->isConnected()) {
        uiLogic()->vpnLogic()->onDisconnect();
    }
    m_settings.removeServer(uiLogic()->selectedServerIndex);

    if (m_settings.defaultServerIndex() == uiLogic()->selectedServerIndex) {
        m_settings.setDefaultServer(0);
    }
    else if (m_settings.defaultServerIndex() > uiLogic()->selectedServerIndex) {
        m_settings.setDefaultServer(m_settings.defaultServerIndex() - 1);
    }

    if (m_settings.serversCount() == 0) {
        m_settings.setDefaultServer(-1);
    }


    uiLogic()->selectedServerIndex = -1;

    uiLogic()->serverListLogic()->updatePage();

    if (m_settings.serversCount() == 0) {
        uiLogic()->setStartPage(Page::Start);
    }
    else {
        uiLogic()->closePage();
    }
}

void ServerSettingsLogic::onPushButtonClearClientCacheClicked()
{
    set_pushButtonClearClientCacheText(tr("Cache cleared"));

    const auto &containers = m_settings.containers(uiLogic()->selectedServerIndex);
    for (DockerContainer container: containers.keys()) {
        m_settings.clearLastConnectionConfig(uiLogic()->selectedServerIndex, container);
    }

    QTimer::singleShot(3000, this, [this]() {
        set_pushButtonClearClientCacheText(tr("Clear client cached profile"));
    });
}

void ServerSettingsLogic::onLineEditDescriptionEditingFinished()
{
    const QString &newText = lineEditDescriptionText();
    QJsonObject server = m_settings.server(uiLogic()->selectedServerIndex);
    server.insert(config_key::description, newText);
    m_settings.editServer(uiLogic()->selectedServerIndex, server);
    uiLogic()->serverListLogic()->updatePage();
}

void ServerSettingsLogic::onPushButtonShareFullClicked()
{
    uiLogic()->shareConnectionLogic()->updateSharingPage(uiLogic()->selectedServerIndex, m_settings.serverCredentials(uiLogic()->selectedServerIndex), DockerContainer::None);
    uiLogic()->goToPage(Page::ShareConnection);
}
