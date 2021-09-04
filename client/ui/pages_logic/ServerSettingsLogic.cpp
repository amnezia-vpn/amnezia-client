//#include <QApplication>
//#include <QClipboard>
//#include <QDebug>
//#include <QDesktopServices>
//#include <QFileDialog>
//#include <QHBoxLayout>
//#include <QHostInfo>
//#include <QItemSelectionModel>
//#include <QJsonDocument>
//#include <QJsonObject>
//#include <QKeyEvent>
//#include <QMenu>
//#include <QMessageBox>
//#include <QMetaEnum>
//#include <QSysInfo>
//#include <QThread>
//#include <QTimer>
//#include <QRegularExpression>
//#include <QSaveFile>


#include "defines.h"
#include "ServerSettingsLogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#include "../uilogic.h"

#include "core/errorstrings.h"
#include <core/servercontroller.h>

using namespace amnezia;
using namespace PageEnumNS;


ServerSettingsLogic::ServerSettingsLogic(UiLogic *uiLogic, QObject *parent):
    QObject(parent),
    m_uiLogic(uiLogic),
    m_pageServerSettingsEnabled{true},
    m_labelServerSettingsWaitInfoVisible{true},
    m_pushButtonServerSettingsClearVisible{true},
    m_pushButtonServerSettingsClearClientCacheVisible{true},
    m_pushButtonServerSettingsShareFullVisible{true},
    m_pushButtonServerSettingsClearText{tr("Clear server from Amnezia software")},
    m_pushButtonServerSettingsClearClientCacheText{tr("Clear client cached profile")}
{

}

void ServerSettingsLogic::updateServerSettingsPage()
{
    setLabelServerSettingsWaitInfoVisible(false);
    setLabelServerSettingsWaitInfoText("");
    setPushButtonServerSettingsClearVisible(m_settings.haveAuthData(m_uiLogic->selectedServerIndex));
    setPushButtonServerSettingsClearClientCacheVisible(m_settings.haveAuthData(m_uiLogic->selectedServerIndex));
    setPushButtonServerSettingsShareFullVisible(m_settings.haveAuthData(m_uiLogic->selectedServerIndex));
    QJsonObject server = m_settings.server(m_uiLogic->selectedServerIndex);
    QString port = server.value(config_key::port).toString();
    setLabelServerSettingsServerText(QString("%1@%2%3%4")
                                     .arg(server.value(config_key::userName).toString())
                                     .arg(server.value(config_key::hostName).toString())
                                     .arg(port.isEmpty() ? "" : ":")
                                     .arg(port));
    setLineEditServerSettingsDescriptionText(server.value(config_key::description).toString());
    QString selectedContainerName = m_settings.defaultContainerName(m_uiLogic->selectedServerIndex);
    setLabelServerSettingsCurrentVpnProtocolText(tr("Protocol: ") + selectedContainerName);
}

bool ServerSettingsLogic::getPageServerSettingsEnabled() const
{
    return m_pageServerSettingsEnabled;
}

void ServerSettingsLogic::setPageServerSettingsEnabled(bool pageServerSettingsEnabled)
{
    if (m_pageServerSettingsEnabled != pageServerSettingsEnabled) {
        m_pageServerSettingsEnabled = pageServerSettingsEnabled;
        emit pageServerSettingsEnabledChanged();
    }
}


void ServerSettingsLogic::onPushButtonServerSettingsClearServer()
{
    setPageServerSettingsEnabled(false);
    setPushButtonServerSettingsClearText(tr("Uninstalling Amnezia software..."));

    if (m_settings.defaultServerIndex() == m_uiLogic->selectedServerIndex) {
        m_uiLogic->onDisconnect();
    }

    ErrorCode e = ServerController::removeAllContainers(m_settings.serverCredentials(m_uiLogic->selectedServerIndex));
    ServerController::disconnectFromHost(m_settings.serverCredentials(m_uiLogic->selectedServerIndex));
    if (e) {
        m_uiLogic->setDialogConnectErrorText(
                    tr("Error occurred while configuring server.") + "\n" +
                    errorString(e) + "\n" +
                    tr("See logs for details."));
        emit m_uiLogic->showConnectErrorDialog();
    }
    else {
        setLabelServerSettingsWaitInfoVisible(true);
        setLabelServerSettingsWaitInfoText(tr("Amnezia server successfully uninstalled"));
    }

    m_settings.setContainers(m_uiLogic->selectedServerIndex, {});
    m_settings.setDefaultContainer(m_uiLogic->selectedServerIndex, DockerContainer::None);

    setPageServerSettingsEnabled(true);
    setPushButtonServerSettingsClearText(tr("Clear server from Amnezia software"));
}

void ServerSettingsLogic::onPushButtonServerSettingsForgetServer()
{
    if (m_settings.defaultServerIndex() == m_uiLogic->selectedServerIndex && m_uiLogic->m_vpnConnection->isConnected()) {
        m_uiLogic->onDisconnect();
    }
    m_settings.removeServer(m_uiLogic->selectedServerIndex);

    if (m_settings.defaultServerIndex() == m_uiLogic->selectedServerIndex) {
        m_settings.setDefaultServer(0);
    }
    else if (m_settings.defaultServerIndex() > m_uiLogic->selectedServerIndex) {
        m_settings.setDefaultServer(m_settings.defaultServerIndex() - 1);
    }

    if (m_settings.serversCount() == 0) {
        m_settings.setDefaultServer(-1);
    }


    m_uiLogic->selectedServerIndex = -1;

    // TODO_REFACT updateServersListPage();

    if (m_settings.serversCount() == 0) {
        m_uiLogic->setStartPage(Page::Start);
    }
    else {
        m_uiLogic->closePage();
    }
}



bool ServerSettingsLogic::getLabelServerSettingsWaitInfoVisible() const
{
    return m_labelServerSettingsWaitInfoVisible;
}

void ServerSettingsLogic::setLabelServerSettingsWaitInfoVisible(bool labelServerSettingsWaitInfoVisible)
{
    if (m_labelServerSettingsWaitInfoVisible != labelServerSettingsWaitInfoVisible) {
        m_labelServerSettingsWaitInfoVisible = labelServerSettingsWaitInfoVisible;
        emit labelServerSettingsWaitInfoVisibleChanged();
    }
}

QString ServerSettingsLogic::getLabelServerSettingsWaitInfoText() const
{
    return m_labelServerSettingsWaitInfoText;
}

void ServerSettingsLogic::setLabelServerSettingsWaitInfoText(const QString &labelServerSettingsWaitInfoText)
{
    if (m_labelServerSettingsWaitInfoText != labelServerSettingsWaitInfoText) {
        m_labelServerSettingsWaitInfoText = labelServerSettingsWaitInfoText;
        emit labelServerSettingsWaitInfoTextChanged();
    }
}

bool ServerSettingsLogic::getPushButtonServerSettingsClearVisible() const
{
    return m_pushButtonServerSettingsClearVisible;
}

void ServerSettingsLogic::setPushButtonServerSettingsClearVisible(bool pushButtonServerSettingsClearVisible)
{
    if (m_pushButtonServerSettingsClearVisible != pushButtonServerSettingsClearVisible) {
        m_pushButtonServerSettingsClearVisible = pushButtonServerSettingsClearVisible;
        emit pushButtonServerSettingsClearVisibleChanged();
    }
}

bool ServerSettingsLogic::getPushButtonServerSettingsClearClientCacheVisible() const
{
    return m_pushButtonServerSettingsClearClientCacheVisible;
}

void ServerSettingsLogic::setPushButtonServerSettingsClearClientCacheVisible(bool pushButtonServerSettingsClearClientCacheVisible)
{
    if (m_pushButtonServerSettingsClearClientCacheVisible != pushButtonServerSettingsClearClientCacheVisible) {
        m_pushButtonServerSettingsClearClientCacheVisible = pushButtonServerSettingsClearClientCacheVisible;
        emit pushButtonServerSettingsClearClientCacheVisibleChanged();
    }
}

bool ServerSettingsLogic::getPushButtonServerSettingsShareFullVisible() const
{
    return m_pushButtonServerSettingsShareFullVisible;
}

void ServerSettingsLogic::setPushButtonServerSettingsShareFullVisible(bool pushButtonServerSettingsShareFullVisible)
{
    if (m_pushButtonServerSettingsShareFullVisible != pushButtonServerSettingsShareFullVisible) {
        m_pushButtonServerSettingsShareFullVisible = pushButtonServerSettingsShareFullVisible;
        emit pushButtonServerSettingsShareFullVisibleChanged();
    }
}

QString ServerSettingsLogic::getLabelServerSettingsServerText() const
{
    return m_labelServerSettingsServerText;
}

void ServerSettingsLogic::setLabelServerSettingsServerText(const QString &labelServerSettingsServerText)
{
    if (m_labelServerSettingsServerText != labelServerSettingsServerText) {
        m_labelServerSettingsServerText = labelServerSettingsServerText;
        emit labelServerSettingsServerTextChanged();
    }
}

QString ServerSettingsLogic::getLineEditServerSettingsDescriptionText() const
{
    return m_lineEditServerSettingsDescriptionText;
}

void ServerSettingsLogic::setLineEditServerSettingsDescriptionText(const QString &lineEditServerSettingsDescriptionText)
{
    if (m_lineEditServerSettingsDescriptionText != lineEditServerSettingsDescriptionText) {
        m_lineEditServerSettingsDescriptionText = lineEditServerSettingsDescriptionText;
        emit lineEditServerSettingsDescriptionTextChanged();
    }
}

QString ServerSettingsLogic::getLabelServerSettingsCurrentVpnProtocolText() const
{
    return m_labelServerSettingsCurrentVpnProtocolText;
}

void ServerSettingsLogic::setLabelServerSettingsCurrentVpnProtocolText(const QString &labelServerSettingsCurrentVpnProtocolText)
{
    if (m_labelServerSettingsCurrentVpnProtocolText != labelServerSettingsCurrentVpnProtocolText) {
        m_labelServerSettingsCurrentVpnProtocolText = labelServerSettingsCurrentVpnProtocolText;
        emit labelServerSettingsCurrentVpnProtocolTextChanged();
    }
}

QString ServerSettingsLogic::getPushButtonServerSettingsClearText() const
{
    return m_pushButtonServerSettingsClearText;
}

void ServerSettingsLogic::setPushButtonServerSettingsClearText(const QString &pushButtonServerSettingsClearText)
{
    if (m_pushButtonServerSettingsClearText != pushButtonServerSettingsClearText) {
        m_pushButtonServerSettingsClearText = pushButtonServerSettingsClearText;
        emit pushButtonServerSettingsClearTextChanged();
    }
}

void ServerSettingsLogic::onPushButtonServerSettingsClearClientCacheClicked()
{
    setPushButtonServerSettingsClearClientCacheText(tr("Cache cleared"));

    const auto &containers = m_settings.containers(m_uiLogic->selectedServerIndex);
    for (DockerContainer container: containers.keys()) {
        m_settings.clearLastConnectionConfig(m_uiLogic->selectedServerIndex, container);
    }

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonServerSettingsClearClientCacheText(tr("Clear client cached profile"));
    });
}

void ServerSettingsLogic::onLineEditServerSettingsDescriptionEditingFinished()
{
    const QString &newText = getLineEditServerSettingsDescriptionText();
    QJsonObject server = m_settings.server(m_uiLogic->selectedServerIndex);
    server.insert(config_key::description, newText);
    m_settings.editServer(m_uiLogic->selectedServerIndex, server);
    // TODO_REFACT updateServersListPage();
}

QString ServerSettingsLogic::getPushButtonServerSettingsClearClientCacheText() const
{
    return m_pushButtonServerSettingsClearClientCacheText;
}

void ServerSettingsLogic::setPushButtonServerSettingsClearClientCacheText(const QString &pushButtonServerSettingsClearClientCacheText)
{
    if (m_pushButtonServerSettingsClearClientCacheText != pushButtonServerSettingsClearClientCacheText) {
        m_pushButtonServerSettingsClearClientCacheText = pushButtonServerSettingsClearClientCacheText;
        emit pushButtonServerSettingsClearClientCacheTextChanged();
    }
}

void ServerSettingsLogic::onPushButtonServerSettingsShareFullClicked()
{
    // TODO_REFACT
    // m_uiLogic->updateSharingPage(m_uiLogic->selectedServerIndex, m_settings.serverCredentials(m_uiLogic->selectedServerIndex), DockerContainer::None);
    m_uiLogic->goToPage(Page::ShareConnection);
}
