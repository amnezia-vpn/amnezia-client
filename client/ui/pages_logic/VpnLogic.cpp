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

//#include "configurators/cloak_configurator.h"
//#include "configurators/vpn_configurator.h"
//#include "configurators/openvpn_configurator.h"
//#include "configurators/shadowsocks_configurator.h"
//#include "configurators/ssh_configurator.h"

//#include "core/servercontroller.h"
//#include "core/server_defs.h"
//#include "core/errorstrings.h"

//#include "protocols/protocols_defs.h"
//#include "protocols/shadowsocksvpnprotocol.h"


#include "VpnLogic.h"

#include "core/errorstrings.h"
#include "vpnconnection.h"
#include <functional>
#include "../uilogic.h"


VpnLogic::VpnLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_pushButtonConnectChecked{false},

    m_radioButtonVpnModeAllSitesChecked{true},
    m_radioButtonVpnModeForwardSitesChecked{false},
    m_radioButtonVpnModeExceptSitesChecked{false},
    m_pushButtonVpnAddSiteEnabled{true},

    m_labelSpeedReceivedText{tr("0 Mbps")},
    m_labelSpeedSentText{tr("0 Mbps")},
    m_labelStateText{},
    m_pushButtonConnectEnabled{false},
    m_widgetVpnModeEnabled{false}
{
    connect(uiLogic()->m_vpnConnection, &VpnConnection::bytesChanged, this, &VpnLogic::onBytesChanged);
    connect(uiLogic()->m_vpnConnection, &VpnConnection::connectionStateChanged, this, &VpnLogic::onConnectionStateChanged);
    connect(uiLogic()->m_vpnConnection, &VpnConnection::vpnProtocolError, this, &VpnLogic::onVpnProtocolError);

    if (m_settings.isAutoConnect() && m_settings.defaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this](){
            setPushButtonConnectEnabled(false);
            onConnect();
        });
    }
}


void VpnLogic::updateVpnPage()
{
    Settings::RouteMode mode = m_settings.routeMode();
    setRadioButtonVpnModeAllSitesChecked(mode == Settings::VpnAllSites);
    setRadioButtonVpnModeForwardSitesChecked(mode == Settings::VpnOnlyForwardSites);
    setRadioButtonVpnModeExceptSitesChecked(mode == Settings::VpnAllExceptSites);
    setPushButtonVpnAddSiteEnabled(mode != Settings::VpnAllSites);
}


void VpnLogic::onRadioButtonVpnModeAllSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void VpnLogic::onRadioButtonVpnModeForwardSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    }
}

void VpnLogic::onRadioButtonVpnModeExceptSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnAllExceptSites);
    }
}

bool VpnLogic::getRadioButtonVpnModeAllSitesChecked() const
{
    return m_radioButtonVpnModeAllSitesChecked;
}

bool VpnLogic::getRadioButtonVpnModeForwardSitesChecked() const
{
    return m_radioButtonVpnModeForwardSitesChecked;
}

bool VpnLogic::getRadioButtonVpnModeExceptSitesChecked() const
{
    return m_radioButtonVpnModeExceptSitesChecked;
}

void VpnLogic::setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked)
{
    if (m_radioButtonVpnModeAllSitesChecked != radioButtonVpnModeAllSitesChecked) {
        m_radioButtonVpnModeAllSitesChecked = radioButtonVpnModeAllSitesChecked;
        emit radioButtonVpnModeAllSitesCheckedChanged();
    }
}

void VpnLogic::setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked)
{
    if (m_radioButtonVpnModeForwardSitesChecked != radioButtonVpnModeForwardSitesChecked) {
        m_radioButtonVpnModeForwardSitesChecked = radioButtonVpnModeForwardSitesChecked;
        emit radioButtonVpnModeForwardSitesCheckedChanged();
    }
}

void VpnLogic::setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked)
{
    if (m_radioButtonVpnModeExceptSitesChecked != radioButtonVpnModeExceptSitesChecked) {
        m_radioButtonVpnModeExceptSitesChecked = radioButtonVpnModeExceptSitesChecked;
        emit radioButtonVpnModeExceptSitesCheckedChanged();
    }
}

bool VpnLogic::getPushButtonConnectChecked() const
{
    return m_pushButtonConnectChecked;
}

void VpnLogic::setPushButtonConnectChecked(bool pushButtonConnectChecked)
{
    if (m_pushButtonConnectChecked != pushButtonConnectChecked) {
        m_pushButtonConnectChecked = pushButtonConnectChecked;
        emit pushButtonConnectCheckedChanged();
    }
}

bool VpnLogic::getPushButtonVpnAddSiteEnabled() const
{
    return m_pushButtonVpnAddSiteEnabled;
}

void VpnLogic::setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled)
{
    if (m_pushButtonVpnAddSiteEnabled != pushButtonVpnAddSiteEnabled) {
        m_pushButtonVpnAddSiteEnabled = pushButtonVpnAddSiteEnabled;
        emit pushButtonVpnAddSiteEnabledChanged();
    }
}

QString VpnLogic::getLabelSpeedReceivedText() const
{
    return m_labelSpeedReceivedText;
}

void VpnLogic::setLabelSpeedReceivedText(const QString &labelSpeedReceivedText)
{
    if (m_labelSpeedReceivedText != labelSpeedReceivedText) {
        m_labelSpeedReceivedText = labelSpeedReceivedText;
        emit labelSpeedReceivedTextChanged();
    }
}

QString VpnLogic::getLabelSpeedSentText() const
{
    return m_labelSpeedSentText;
}

void VpnLogic::setLabelSpeedSentText(const QString &labelSpeedSentText)
{
    if (m_labelSpeedSentText != labelSpeedSentText) {
        m_labelSpeedSentText = labelSpeedSentText;
        emit labelSpeedSentTextChanged();
    }
}

QString VpnLogic::getLabelStateText() const
{
    return m_labelStateText;
}

void VpnLogic::setLabelStateText(const QString &labelStateText)
{
    if (m_labelStateText != labelStateText) {
        m_labelStateText = labelStateText;
        emit labelStateTextChanged();
    }
}

bool VpnLogic::getPushButtonConnectEnabled() const
{
    return m_pushButtonConnectEnabled;
}

void VpnLogic::setPushButtonConnectEnabled(bool pushButtonConnectEnabled)
{
    if (m_pushButtonConnectEnabled != pushButtonConnectEnabled) {
        m_pushButtonConnectEnabled = pushButtonConnectEnabled;
        emit pushButtonConnectEnabledChanged();
    }
}

bool VpnLogic::getWidgetVpnModeEnabled() const
{
    return m_widgetVpnModeEnabled;
}

void VpnLogic::setWidgetVpnModeEnabled(bool widgetVpnModeEnabled)
{
    if (m_widgetVpnModeEnabled != widgetVpnModeEnabled) {
        m_widgetVpnModeEnabled = widgetVpnModeEnabled;
        emit widgetVpnModeEnabledChanged();
    }
}

QString VpnLogic::getLabelErrorText() const
{
    return m_labelErrorText;
}

void VpnLogic::setLabelErrorText(const QString &labelErrorText)
{
    if (m_labelErrorText != labelErrorText) {
        m_labelErrorText = labelErrorText;
        emit labelErrorTextChanged();
    }
}

void VpnLogic::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    setLabelSpeedReceivedText(VpnConnection::bytesPerSecToText(receivedData));
    setLabelSpeedSentText(VpnConnection::bytesPerSecToText(sentData));
}

void VpnLogic::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    qDebug() << "UiLogic::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);

    bool pushButtonConnectEnabled = false;
    bool radioButtonsModeEnabled = false;
    setLabelStateText(VpnProtocol::textConnectionState(state));

    uiLogic()->setTrayState(state);

    switch (state) {
    case VpnProtocol::Disconnected:
        onBytesChanged(0,0);
        setPushButtonConnectChecked(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::Preparing:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Connecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Connected:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Disconnecting:
        pushButtonConnectEnabled = false;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Reconnecting:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = false;
        break;
    case VpnProtocol::Error:
        setPushButtonConnectEnabled(false);
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
        break;
    case VpnProtocol::Unknown:
        pushButtonConnectEnabled = true;
        radioButtonsModeEnabled = true;
    }

    setPushButtonConnectEnabled(pushButtonConnectEnabled);
    setWidgetVpnModeEnabled(radioButtonsModeEnabled);
}

void VpnLogic::onVpnProtocolError(ErrorCode errorCode)
{
    setLabelErrorText(errorString(errorCode));
}

void VpnLogic::onPushButtonConnectClicked(bool checked)
{
    if (checked) {
        onConnect();
    } else {
        onDisconnect();
    }
}

void VpnLogic::onConnect()
{
    int serverIndex = m_settings.defaultServerIndex();
    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);
    DockerContainer container = m_settings.defaultContainer(serverIndex);

    if (m_settings.containers(serverIndex).isEmpty()) {
        setLabelErrorText(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
        setPushButtonConnectChecked(false);
        return;
    }

    if (container == DockerContainer::None) {
        setLabelErrorText(tr("VPN Protocol not choosen"));
        setPushButtonConnectChecked(false);
        return;
    }


    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);
    onConnectWorker(serverIndex, credentials, container, containerConfig);
}

void VpnLogic::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    setLabelErrorText("");
    setPushButtonConnectChecked(true);
    qApp->processEvents();

    ErrorCode errorCode = uiLogic()->m_vpnConnection->connectToVpn(
                serverIndex, credentials, container, containerConfig
                );

    if (errorCode) {
        //ui->pushButton_connect->setChecked(false);
        uiLogic()->setDialogConnectErrorText(errorString(errorCode));
        emit uiLogic()->showConnectErrorDialog();
        return;
    }

    setPushButtonConnectEnabled(false);
}

void VpnLogic::onDisconnect()
{
    setPushButtonConnectChecked(false);
    uiLogic()->m_vpnConnection->disconnectFromVpn();
}
