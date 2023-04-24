#include <QApplication>

#include "VpnLogic.h"

#include "core/errorstrings.h"
#include "vpnconnection.h"
#include <QTimer>
#include <functional>
#include "../uilogic.h"
#include "defines.h"
#include <configurators/vpn_configurator.h>


VpnLogic::VpnLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonConnectChecked{false},

    m_radioButtonVpnModeAllSitesChecked{true},
    m_radioButtonVpnModeForwardSitesChecked{false},
    m_radioButtonVpnModeExceptSitesChecked{false},

    m_labelSpeedReceivedText{tr("0 Mbps")},
    m_labelSpeedSentText{tr("0 Mbps")},
    m_labelStateText{},
    m_isContainerHaveAuthData{false},
    m_isContainerSupportedByCurrentPlatform{false},
    m_widgetVpnModeEnabled{false}
{
    connect(uiLogic()->m_vpnConnection, &VpnConnection::bytesChanged, this, &VpnLogic::onBytesChanged);
    connect(uiLogic()->m_vpnConnection, &VpnConnection::connectionStateChanged, this, &VpnLogic::onConnectionStateChanged);
    connect(uiLogic()->m_vpnConnection, &VpnConnection::vpnProtocolError, this, &VpnLogic::onVpnProtocolError);

    connect(this, &VpnLogic::connectToVpn, uiLogic()->m_vpnConnection, &VpnConnection::connectToVpn, Qt::QueuedConnection);
    connect(this, &VpnLogic::disconnectFromVpn, uiLogic()->m_vpnConnection, &VpnConnection::disconnectFromVpn, Qt::QueuedConnection);

    connect(m_settings.get(), &Settings::saveLogsChanged, this, &VpnLogic::onUpdatePage);

    if (m_settings->isAutoConnect() && m_settings->defaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this](){
            set_pushButtonConnectEnabled(false);
            onConnect();
        });
    }
    else {
        onConnectionStateChanged(VpnProtocol::Disconnected);
    }
}


void VpnLogic::onUpdatePage()
{
    Settings::RouteMode mode = m_settings->routeMode();
    DockerContainer selectedContainer = m_settings->defaultContainer(m_settings->defaultServerIndex());

    set_isCustomRoutesSupported  (selectedContainer == DockerContainer::OpenVpn ||
                                  selectedContainer == DockerContainer::ShadowSocks||
                                  selectedContainer == DockerContainer::Cloak);

    set_isContainerHaveAuthData(m_settings->haveAuthData(m_settings->defaultServerIndex()));

    set_radioButtonVpnModeAllSitesChecked(mode == Settings::VpnAllSites || !isCustomRoutesSupported());
    set_radioButtonVpnModeForwardSitesChecked(mode == Settings::VpnOnlyForwardSites && isCustomRoutesSupported());
    set_radioButtonVpnModeExceptSitesChecked(mode == Settings::VpnAllExceptSites && isCustomRoutesSupported());

    const QJsonObject &server = uiLogic()->m_settings->defaultServer();
    QString serverString = QString("%2 (%3)")
            .arg(server.value(config_key::description).toString())
            .arg(server.value(config_key::hostName).toString());
    set_labelCurrentServer(serverString);

    QString selectedContainerName = ContainerProps::containerHumanNames().value(selectedContainer);
    set_labelCurrentService(selectedContainerName);

    auto dns = m_configurator->getDnsForConfig(m_settings->defaultServerIndex());
    set_amneziaDnsEnabled(dns.first == protocols::dns::amneziaDnsIp);
    if (dns.first == protocols::dns::amneziaDnsIp) {
        set_labelCurrentDns("On your server");
    }
    else {
        set_labelCurrentDns(dns.first + ", " + dns.second);
    }


    set_isContainerSupportedByCurrentPlatform(ContainerProps::isSupportedByCurrentPlatform(selectedContainer));
    if (!isContainerSupportedByCurrentPlatform()) {
        set_labelErrorText(tr("AmneziaVPN not supporting selected protocol on this device. Select another protocol."));
    }
    else {
        set_labelErrorText("");
    }
    QString ver = QString("v. %2").arg(QString(APP_MAJOR_VERSION));
    set_labelVersionText(ver);

    set_labelLogEnabledVisible(m_settings->isSaveLogs());
}


void VpnLogic::onRadioButtonVpnModeAllSitesClicked()
{
    m_settings->setRouteMode(Settings::VpnAllSites);
    onUpdatePage();
}

void VpnLogic::onRadioButtonVpnModeForwardSitesClicked()
{
    m_settings->setRouteMode(Settings::VpnOnlyForwardSites);
    onUpdatePage();
}

void VpnLogic::onRadioButtonVpnModeExceptSitesClicked()
{
    m_settings->setRouteMode(Settings::VpnAllExceptSites);
    onUpdatePage();
}

void VpnLogic::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    set_labelSpeedReceivedText(VpnConnection::bytesPerSecToText(receivedData));
    set_labelSpeedSentText(VpnConnection::bytesPerSecToText(sentData));
}

void VpnLogic::onConnectionStateChanged(VpnProtocol::VpnConnectionState state)
{
    qDebug() << "VpnLogic::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);
    if (uiLogic()->m_vpnConnection == NULL) {
        qDebug() << "VpnLogic::onConnectionStateChanged" << VpnProtocol::textConnectionState(state) << "невозможно, соединение отсутствует (уничтожено ранее)";
        return;
    }
    bool pbConnectEnabled = false;
    bool pbConnectChecked = false;

    bool rbModeEnabled = false;
    bool pbConnectVisible = false;
    set_labelStateText(VpnProtocol::textConnectionState(state));

    switch (state) {
    case VpnProtocol::Disconnected:
        onBytesChanged(0,0);
        pbConnectChecked = false;
        pbConnectEnabled = true;
        pbConnectVisible = true;
        rbModeEnabled = true;
        break;
    case VpnProtocol::Preparing:
        pbConnectChecked = true;
        pbConnectEnabled = false;
        pbConnectVisible = false;
        rbModeEnabled = false;
        break;
    case VpnProtocol::Connecting:
        pbConnectChecked = true;
        pbConnectEnabled = false;
        pbConnectVisible = false;
        rbModeEnabled = false;
        break;
    case VpnProtocol::Connected:
        pbConnectChecked = true;
        pbConnectEnabled = true;
        pbConnectVisible = true;
        rbModeEnabled = false;
        break;
    case VpnProtocol::Disconnecting:
        pbConnectChecked = false;
        pbConnectEnabled = false;
        pbConnectVisible = false;
        rbModeEnabled = false;
        break;
    case VpnProtocol::Reconnecting:
        pbConnectChecked = true;
        pbConnectEnabled = true;
        pbConnectVisible = false;
        rbModeEnabled = false;
        break;
    case VpnProtocol::Error:
        pbConnectChecked = false;
        pbConnectEnabled = true;
        pbConnectVisible = true;
        rbModeEnabled = true;
        break;
    case VpnProtocol::Unknown:
        pbConnectChecked = false;
        pbConnectEnabled = true;
        pbConnectVisible = true;
        rbModeEnabled = true;
    }

    set_pushButtonConnectEnabled(pbConnectEnabled);
    set_pushButtonConnectChecked(pbConnectChecked);

    set_pushButtonConnectVisible(pbConnectVisible);
    set_widgetVpnModeEnabled(rbModeEnabled);
}

void VpnLogic::onVpnProtocolError(ErrorCode errorCode)
{
    set_labelErrorText(errorString(errorCode));
}

void VpnLogic::onPushButtonConnectClicked()
{
    if (! pushButtonConnectChecked()) {
        onConnect();
    } else {
        onDisconnect();
    }
}

void VpnLogic::onConnect()
{
    int serverIndex = m_settings->defaultServerIndex();
    ServerCredentials credentials = m_settings->serverCredentials(serverIndex);
    DockerContainer container = m_settings->defaultContainer(serverIndex);

    if (m_settings->containers(serverIndex).isEmpty()) {
        set_labelErrorText(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        set_pushButtonConnectChecked(false);
        return;
    }

    if (container == DockerContainer::None) {
        set_labelErrorText(tr("VPN Protocol not chosen"));
        set_pushButtonConnectChecked(false);
        return;
    }


    const QJsonObject &containerConfig = m_settings->containerConfig(serverIndex, container);
    onConnectWorker(serverIndex, credentials, container, containerConfig);
}

void VpnLogic::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    set_labelErrorText("");
    set_pushButtonConnectChecked(true);
    set_pushButtonConnectEnabled(false);

    qApp->processEvents();

    emit connectToVpn(serverIndex, credentials, container, containerConfig);
}

void VpnLogic::onDisconnect()
{
    onConnectionStateChanged(VpnProtocol::Disconnected);
    emit disconnectFromVpn();
}
