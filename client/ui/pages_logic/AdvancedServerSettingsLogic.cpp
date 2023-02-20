#include "AdvancedServerSettingsLogic.h"

#include "VpnLogic.h"
#include "ui/uilogic.h"
#include "core/errorstrings.h"
#include "core/servercontroller.h"

AdvancedServerSettingsLogic::AdvancedServerSettingsLogic(UiLogic *uiLogic, QObject *parent): PageLogicBase(uiLogic, parent),
      m_labelWaitInfoVisible{true},
      m_pushButtonClearVisible{true},
      m_pushButtonClearText{tr("Clear server from Amnezia software")}
{
}

void AdvancedServerSettingsLogic::onUpdatePage()
{
    set_labelWaitInfoVisible(false);
    set_labelWaitInfoText("");
    set_pushButtonClearVisible(m_settings->haveAuthData(uiLogic()->m_selectedServerIndex));
    const QJsonObject &server = m_settings->server(uiLogic()->m_selectedServerIndex);
    const QString &port = server.value(config_key::port).toString();

    const QString &userName = server.value(config_key::userName).toString();
    const QString &hostName = server.value(config_key::hostName).toString();
    QString name = QString("%1%2%3%4%5").arg(userName,
                                             userName.isEmpty() ? "" : "@",
                                             hostName,
                                             port.isEmpty() ? "" : ":",
                                             port);

    set_labelServerText(name);

    DockerContainer selectedContainer = m_settings->defaultContainer(uiLogic()->m_selectedServerIndex);
    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentVpnProtocolText(tr("Service: ") + selectedContainerName);
}

void AdvancedServerSettingsLogic::onPushButtonClearServer()
{
    set_pageEnabled(false);
    set_pushButtonClearText(tr("Uninstalling Amnezia software..."));

    if (m_settings->defaultServerIndex() == uiLogic()->m_selectedServerIndex) {
        uiLogic()->pageLogic<VpnLogic>()->onDisconnect();
    }

    ErrorCode e = m_serverController->removeAllContainers(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex));
    m_serverController->disconnectFromHost(m_settings->serverCredentials(uiLogic()->m_selectedServerIndex));
    if (e) {
        uiLogic()->set_dialogConnectErrorText(tr("Error occurred while configuring server.") + "\n" +
                                              errorString(e) + "\n" + tr("See logs for details."));
        emit uiLogic()->showConnectErrorDialog();
    } else {
        set_labelWaitInfoVisible(true);
        set_labelWaitInfoText(tr("Amnezia server successfully uninstalled"));
    }

    m_settings->setContainers(uiLogic()->m_selectedServerIndex, {});
    m_settings->setDefaultContainer(uiLogic()->m_selectedServerIndex, DockerContainer::None);

    set_pageEnabled(true);
    set_pushButtonClearText(tr("Clear server from Amnezia software"));
}
