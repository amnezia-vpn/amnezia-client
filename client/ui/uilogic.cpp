#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QHostInfo>
#include <QItemSelectionModel>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMetaEnum>
#include <QSysInfo>
#include <QThread>
#include <QTimer>
#include <QRegularExpression>
#include <QSaveFile>

#include "configurators/cloak_configurator.h"
#include "configurators/vpn_configurator.h"
#include "configurators/openvpn_configurator.h"
#include "configurators/shadowsocks_configurator.h"
#include "configurators/ssh_configurator.h"

#include "core/servercontroller.h"
#include "core/server_defs.h"
#include "core/errorstrings.h"

#include "protocols/protocols_defs.h"
#include "protocols/shadowsocksvpnprotocol.h"

#include "ui/qautostart.h"

#include "debug.h"
#include "defines.h"
#include "uilogic.h"
#include "utils.h"
#include "vpnconnection.h"
#include <functional>

#if defined Q_OS_MAC || defined Q_OS_LINUX
#include "ui/macos_util.h"
#endif

using namespace amnezia;
using namespace PageEnumNS;

UiLogic::UiLogic(QObject *parent) :
    QObject(parent),
    m_frameWireguardSettingsVisible{false},
    m_frameWireguardVisible{false},
    m_frameNewServerSettingsParentWireguardVisible{false},
    m_radioButtonSetupWizardMediumChecked{true},
    m_lineEditSetupWizardHighWebsiteMaskingText{},
    m_progressBarNewServerConfiguringValue{0},
    m_pushButtonNewServerSettingsCloakChecked{false},
    m_pushButtonNewServerSettingsSsChecked{false},
    m_pushButtonNewServerSettingsOpenvpnChecked{false},
    m_lineEditNewServerCloakPortText{},
    m_lineEditNewServerCloakSiteText{},
    m_lineEditNewServerSsPortText{},
    m_comboBoxNewServerSsCipherText{"chacha20-ietf-poly1305"},
    m_lineEditNewServerOpenvpnPortText{},
    m_comboBoxNewServerOpenvpnProtoText{"udp"},
    m_pushButtonNewServerConnectKeyChecked{false},
    m_lineEditStartExistingCodeText{},
    m_textEditNewServerSshKeyText{},
    m_lineEditNewServerIpText{},
    m_lineEditNewServerPasswordText{},
    m_lineEditNewServerLoginText{},
    m_labelNewServerWaitInfoVisible{true},
    m_labelNewServerWaitInfoText{},
    m_progressBarNewServerConnectionMinimum{0},
    m_progressBarNewServerConnectionMaximum{100},
    m_pushButtonBackFromStartVisible{true},
    m_pushButtonNewServerConnectVisible{true},
    m_radioButtonVpnModeAllSitesChecked{true},
    m_radioButtonVpnModeForwardSitesChecked{false},
    m_radioButtonVpnModeExceptSitesChecked{false},
    m_pushButtonVpnAddSiteEnabled{true},

    m_labelServerSettingsWaitInfoVisible{true},
    m_labelServerSettingsWaitInfoText{},
    m_pushButtonServerSettingsClearVisible{true},
    m_pushButtonServerSettingsClearClientCacheVisible{true},
    m_pushButtonServerSettingsShareFullVisible{true},
    m_labelServerSettingsServerText{},
    m_lineEditServerSettingsDescriptionText{},
    m_labelServerSettingsCurrentVpnProtocolText{},
    m_currentPageValue{0},
    m_trayIconUrl{},
    m_trayActionDisconnectEnabled{true},
    m_trayActionConnectEnabled{true},
    m_checkBoxNewServerCloakChecked{true},
    m_checkBoxNewServerSsChecked{false},
    m_checkBoxNewServerOpenvpnChecked{false},
    m_comboBoxProtoCloakCipherText{"chacha20-poly1305"},
    m_lineEditProtoCloakSiteText{"tile.openstreetmap.org"},
    m_lineEditProtoCloakPortText{},
    m_comboBoxProtoShadowsocksCipherText{"chacha20-poly1305"},
    m_lineEditProtoShadowsocksPortText{},
    m_lineEditProtoOpenvpnSubnetText{},
    m_radioButtonProtoOpenvpnUdpChecked{false},
    m_checkBoxProtoOpenvpnAutoEncryptionChecked{},
    m_comboBoxProtoOpenvpnCipherText{"AES-256-GCM"},
    m_comboBoxProtoOpenvpnHashText{"SHA512"},
    m_checkBoxProtoOpenvpnBlockDnsChecked{false},
    m_lineEditProtoOpenvpnPortText{},
    m_checkBoxProtoOpenvpnTlsAuthChecked{false},
    m_radioButtonSetupWizardHighChecked{false},
    m_radioButtonSetupWizardLowChecked{false},
    m_checkBoxSetupWizardVpnModeChecked{false},
    m_pushButtonConnectChecked{false},

    m_widgetProtoCloakEnabled{false},
    m_pushButtonProtoCloakSaveVisible{false},
    m_progressBarProtoCloakResetVisible{false},
    m_lineEditProtoCloakPortEnabled{false},
    m_widgetProtoSsEnabled{false},
    m_pushButtonProtoShadowsocksSaveVisible{false},
    m_progressBarProtoShadowsocksResetVisible{false},
    m_lineEditProtoShadowsocksPortEnabled{false},
    m_widgetProtoOpenvpnEnabled{false},
    m_pushButtonProtoOpenvpnSaveVisible{false},
    m_progressBarProtoOpenvpnResetVisible{false},
    m_radioButtonProtoOpenvpnUdpEnabled{false},
    m_radioButtonProtoOpenvpnTcpEnabled{false},
    m_radioButtonProtoOpenvpnTcpChecked{false},
    m_lineEditProtoOpenvpnPortEnabled{false},
    m_pushButtonProtoOpenvpnContInstallChecked{false},
    m_pushButtonProtoSsOpenvpnContInstallChecked{false},
    m_pushButtonProtoCloakOpenvpnContInstallChecked{false},
    m_pushButtonProtoWireguardContInstallChecked{false},
    m_pushButtonProtoOpenvpnContInstallEnabled{false},
    m_pushButtonProtoSsOpenvpnContInstallEnabled{false},
    m_pushButtonProtoCloakOpenvpnContInstallEnabled{false},
    m_pushButtonProtoWireguardContInstallEnabled{false},
    m_pushButtonProtoOpenvpnContDefaultChecked{false},
    m_pushButtonProtoSsOpenvpnContDefaultChecked{false},
    m_pushButtonProtoCloakOpenvpnContDefaultChecked{false},
    m_pushButtonProtoWireguardContDefaultChecked{false},
    m_pushButtonProtoOpenvpnContDefaultVisible{true},
    m_pushButtonProtoSsOpenvpnContDefaultVisible{false},
    m_pushButtonProtoCloakOpenvpnContDefaultVisible{false},
    m_pushButtonProtoWireguardContDefaultVisible{false},
    m_pushButtonProtoOpenvpnContShareVisible{false},
    m_pushButtonProtoSsOpenvpnContShareVisible{false},
    m_pushButtonProtoCloakOpenvpnContShareVisible{false},
    m_pushButtonProtoWireguardContShareVisible{false},
    m_frameOpenvpnSettingsVisible{true},
    m_frameOpenvpnSsSettingsVisible{true},
    m_frameOpenvpnSsCloakSettingsVisible{true},
    m_progressBarProtocolsContainerReinstallVisible{false},
    m_labelSpeedReceivedText{tr("0 Mbps")},
    m_labelSpeedSentText{tr("0 Mbps")},
    m_labelStateText{},
    m_pushButtonConnectEnabled{false},
    m_widgetVpnModeEnabled{false},
    m_labelErrorText{tr("Error text")},
    m_pushButtonNewServerConnectEnabled{},
    m_pushButtonNewServerConnectText{tr("Connect")},
    m_dialogConnectErrorText{},
    m_pageServerSettingsEnabled{true},
    m_pushButtonServerSettingsClearText{tr("Clear server from Amnezia software")},
    m_pageShareAmneziaVisible{true},
    m_pageShareOpenvpnVisible{true},
    m_pageShareShadowsocksVisible{true},
    m_pageShareCloakVisible{true},
    m_pageShareFullAccessVisible{true},
    m_textEditShareOpenvpnCodeText{},
    m_pushButtonShareOpenvpnCopyEnabled{false},
    m_pushButtonShareOpenvpnSaveEnabled{false},
    m_toolBoxShareConnectionCurrentIndex{-1},
    m_pushButtonShareSsCopyEnabled{false},
    m_lineEditShareSsStringText{},
    m_labelShareSsQrCodeText{},
    m_labelShareSsServerText{},
    m_labelShareSsPortText{},
    m_labelShareSsMethodText{},
    m_labelShareSsPasswordText{},
    m_plainTextEditShareCloakText{},
    m_pushButtonShareCloakCopyEnabled{false},
    m_textEditShareFullCodeText{},
    m_textEditShareAmneziaCodeText{},
    m_pushButtonShareFullCopyText{tr("Copy")},
    m_pushButtonShareAmneziaCopyText{tr("Copy")},
    m_pushButtonShareOpenvpnCopyText{tr("Copy")},
    m_pushButtonShareSsCopyText{tr("Copy")},
    m_pushButtonShareCloakCopyText{tr("Copy")},
    m_pushButtonShareAmneziaGenerateEnabled{true},
    m_pushButtonShareAmneziaCopyEnabled{true},
    m_pushButtonShareAmneziaGenerateText{tr("Generate config")},
    m_pushButtonShareOpenvpnGenerateEnabled{true},
    m_pushButtonShareOpenvpnGenerateText{tr("Generate config")},
    m_pageNewServerConfiguringEnabled{true},
    m_labelNewServerConfiguringWaitInfoVisible{true},
    m_labelNewServerConfiguringWaitInfoText{tr("Please wait, configuring process may take up to 5 minutes")},
    m_progressBarNewServerConfiguringVisible{true},
    m_progressBarNewServerConfiguringMaximium{100},
    m_progressBarNewServerConfiguringTextVisible{true},
    m_progressBarNewServerConfiguringText{tr("Configuring...")},
    m_pageServerProtocolsEnabled{true},
    m_progressBarProtocolsContainerReinstallValue{0},
    m_progressBarProtocolsContainerReinstallMaximium{100},
    m_comboBoxProtoOpenvpnCipherEnabled{true},
    m_comboBoxProtoOpenvpnHashEnabled{true},
    m_pageProtoOpenvpnEnabled{true},
    m_labelProtoOpenvpnInfoVisible{true},
    m_labelProtoOpenvpnInfoText{},
    m_progressBarProtoOpenvpnResetValue{0},
    m_progressBarProtoOpenvpnResetMaximium{100},
    m_pageProtoShadowsocksEnabled{true},
    m_labelProtoShadowsocksInfoVisible{true},
    m_labelProtoShadowsocksInfoText{},
    m_progressBarProtoShadowsocksResetValue{0},
    m_progressBarProtoShadowsocksResetMaximium{100},
    m_pageProtoCloakEnabled{true},
    m_labelProtoCloakInfoVisible{true},
    m_labelProtoCloakInfoText{},
    m_progressBarProtoCloakResetValue{0},
    m_progressBarProtoCloakResetMaximium{100},
    m_serverListModel{nullptr},
    m_pushButtonServerSettingsClearClientCacheText{tr("Clear client cached profile")},
    m_vpnConnection(nullptr)
{
    m_vpnConnection = new VpnConnection(this);
    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    connect(m_vpnConnection, SIGNAL(vpnProtocolError(amnezia::ErrorCode)), this, SLOT(onVpnProtocolError(amnezia::ErrorCode)));
}

void UiLogic::initalizeUiLogic()
{
    setFrameWireguardSettingsVisible(false);
    setFrameWireguardVisible(false);
    setFrameNewServerSettingsParentWireguardVisible(false);

    setupTray();
    setupNewServerConnections();
    setupProtocolsPageConnections();

    setLabelErrorText("");


    //    if (QOperatingSystemVersion::current() <= QOperatingSystemVersion::Windows7) {
    //        needToHideCustomTitlebar = true;
    //    }

    //#if defined Q_OS_MAC
    //    fixWidget(this);
    //    needToHideCustomTitlebar = true;
    //#endif

    //    if (needToHideCustomTitlebar) {
    //        ui->widget_tittlebar->hide();
    //        resize(width(), 640);
    //        ui->stackedWidget_main->move(0,0);
    //    }

    // Post initialization
    goToPage(Page::Start, true, false);

    if (m_settings.defaultServerIndex() >= 0 && m_settings.serversCount() > 0) {
        goToPage(Page::Vpn, true, false);
    }

    //    //ui->pushButton_general_settings_exit->hide();
    updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);


    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());



    onConnectionStateChanged(VpnProtocol::Disconnected);

    if (m_settings.isAutoConnect() && m_settings.defaultServerIndex() >= 0) {
        QTimer::singleShot(1000, this, [this](){
            setPushButtonConnectEnabled(false);
            onConnect();
        });
    }

    //    m_ipAddressValidator.setRegExp(Utils::ipAddressRegExp());
    //    m_ipAddressPortValidator.setRegExp(Utils::ipAddressPortRegExp());
    //    m_ipNetwok24Validator.setRegExp(Utils::ipNetwork24RegExp());
    //    m_ipPortValidator.setRegExp(Utils::ipPortRegExp());

    //    ui->lineEdit_new_server_ip->setValidator(&m_ipAddressPortValidator);
    //    ui->lineEdit_network_settings_dns1->setValidator(&m_ipAddressValidator);
    //    ui->lineEdit_network_settings_dns2->setValidator(&m_ipAddressValidator);

    //    ui->lineEdit_proto_openvpn_subnet->setValidator(&m_ipNetwok24Validator);

    //    ui->lineEdit_proto_openvpn_port->setValidator(&m_ipPortValidator);
    //    ui->lineEdit_proto_shadowsocks_port->setValidator(&m_ipPortValidator);
    //    ui->lineEdit_proto_cloak_port->setValidator(&m_ipPortValidator);




    m_serverListModel = new ServersModel(this);
}

bool UiLogic::getFrameWireguardSettingsVisible() const
{
    return m_frameWireguardSettingsVisible;
}

void UiLogic::setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible)
{
    if (m_frameWireguardSettingsVisible != frameWireguardSettingsVisible) {
        m_frameWireguardSettingsVisible = frameWireguardSettingsVisible;
        emit frameWireguardSettingsVisibleChanged();
    }
}

bool UiLogic::getFrameWireguardVisible() const
{
    return m_frameWireguardVisible;
}

void UiLogic::setFrameWireguardVisible(bool frameWireguardVisible)
{
    if (m_frameWireguardVisible != frameWireguardVisible) {
        m_frameWireguardVisible = frameWireguardVisible;
        emit frameWireguardVisibleChanged();
    }
}

bool UiLogic::getFrameNewServerSettingsParentWireguardVisible() const
{
    return m_frameNewServerSettingsParentWireguardVisible;
}

void UiLogic::setFrameNewServerSettingsParentWireguardVisible(bool frameNewServerSettingsParentWireguardVisible)
{
    if (m_frameNewServerSettingsParentWireguardVisible != frameNewServerSettingsParentWireguardVisible) {
        m_frameNewServerSettingsParentWireguardVisible = frameNewServerSettingsParentWireguardVisible;
        emit frameNewServerSettingsParentWireguardVisibleChanged();
    }
}

bool UiLogic::getRadioButtonSetupWizardMediumChecked() const
{
    return m_radioButtonSetupWizardMediumChecked;
}

void UiLogic::setRadioButtonSetupWizardMediumChecked(bool radioButtonSetupWizardMediumChecked)
{
    if (m_radioButtonSetupWizardMediumChecked != radioButtonSetupWizardMediumChecked) {
        m_radioButtonSetupWizardMediumChecked = radioButtonSetupWizardMediumChecked;
        emit radioButtonSetupWizardMediumCheckedChanged();
    }
}

void UiLogic::updateWizardHighPage()
{
    setLineEditSetupWizardHighWebsiteMaskingText(protocols::cloak::defaultRedirSite);
}

void UiLogic::updateNewServerProtocolsPage()
{
    setPushButtonNewServerSettingsCloakChecked(true);
    setPushButtonNewServerSettingsCloakChecked(false);
    setPushButtonNewServerSettingsSsChecked(true);
    setPushButtonNewServerSettingsSsChecked(false);
    setLineEditNewServerCloakPortText(amnezia::protocols::cloak::defaultPort);
    setLineEditNewServerCloakSiteText(amnezia::protocols::cloak::defaultRedirSite);
    setLineEditNewServerSsPortText(amnezia::protocols::shadowsocks::defaultPort);
    setComboBoxNewServerSsCipherText(amnezia::protocols::shadowsocks::defaultCipher);
    setLineEditNewServerOpenvpnPortText(amnezia::protocols::openvpn::defaultPort);
    setComboBoxNewServerOpenvpnProtoText(amnezia::protocols::openvpn::defaultTransportProto);
}

bool UiLogic::getPushButtonNewServerConnectKeyChecked() const
{
    return m_pushButtonNewServerConnectKeyChecked;
}

void UiLogic::setPushButtonNewServerConnectKeyChecked(bool pushButtonNewServerConnectKeyChecked)
{
    if (m_pushButtonNewServerConnectKeyChecked != pushButtonNewServerConnectKeyChecked) {
        m_pushButtonNewServerConnectKeyChecked = pushButtonNewServerConnectKeyChecked;
        emit pushButtonNewServerConnectKeyCheckedChanged();
    }
}

QString UiLogic::getComboBoxNewServerOpenvpnProtoText() const
{
    return m_comboBoxNewServerOpenvpnProtoText;
}

void UiLogic::setComboBoxNewServerOpenvpnProtoText(const QString &comboBoxNewServerOpenvpnProtoText)
{
    if (m_comboBoxNewServerOpenvpnProtoText != comboBoxNewServerOpenvpnProtoText) {
        m_comboBoxNewServerOpenvpnProtoText = comboBoxNewServerOpenvpnProtoText;
        emit comboBoxNewServerOpenvpnProtoTextChanged();
    }
}

QString UiLogic::getLineEditNewServerCloakSiteText() const
{
    return m_lineEditNewServerCloakSiteText;
}

void UiLogic::setLineEditNewServerCloakSiteText(const QString &lineEditNewServerCloakSiteText)
{
    if (m_lineEditNewServerCloakSiteText != lineEditNewServerCloakSiteText) {
        m_lineEditNewServerCloakSiteText = lineEditNewServerCloakSiteText;
        emit lineEditNewServerCloakSiteTextChanged();
    }
}

QString UiLogic::getLineEditNewServerSsPortText() const
{
    return m_lineEditNewServerSsPortText;
}

void UiLogic::setLineEditNewServerSsPortText(const QString &lineEditNewServerSsPortText)
{
    if (m_lineEditNewServerSsPortText != lineEditNewServerSsPortText) {
        m_lineEditNewServerSsPortText = lineEditNewServerSsPortText;
        emit lineEditNewServerSsPortTextChanged();
    }
}

QString UiLogic::getComboBoxNewServerSsCipherText() const
{
    return m_comboBoxNewServerSsCipherText;
}

void UiLogic::setComboBoxNewServerSsCipherText(const QString &comboBoxNewServerSsCipherText)
{
    if (m_comboBoxNewServerSsCipherText != comboBoxNewServerSsCipherText) {
        m_comboBoxNewServerSsCipherText = comboBoxNewServerSsCipherText;
        emit comboBoxNewServerSsCipherTextChanged();
    }
}

QString UiLogic::getlineEditNewServerOpenvpnPortText() const
{
    return m_lineEditNewServerOpenvpnPortText;
}

void UiLogic::setLineEditNewServerOpenvpnPortText(const QString &lineEditNewServerOpenvpnPortText)
{
    if (m_lineEditNewServerOpenvpnPortText != lineEditNewServerOpenvpnPortText) {
        m_lineEditNewServerOpenvpnPortText = lineEditNewServerOpenvpnPortText;
        emit lineEditNewServerOpenvpnPortTextChanged();
    }
}

bool UiLogic::getPushButtonNewServerSettingsSsChecked() const
{
    return m_pushButtonNewServerSettingsSsChecked;
}

void UiLogic::setPushButtonNewServerSettingsSsChecked(bool pushButtonNewServerSettingsSsChecked)
{
    if (m_pushButtonNewServerSettingsSsChecked != pushButtonNewServerSettingsSsChecked) {
        m_pushButtonNewServerSettingsSsChecked = pushButtonNewServerSettingsSsChecked;
        emit pushButtonNewServerSettingsSsCheckedChanged();
    }
}

bool UiLogic::getPushButtonNewServerSettingsOpenvpnChecked() const
{
    return m_pushButtonNewServerSettingsOpenvpnChecked;
}

void UiLogic::setPushButtonNewServerSettingsOpenvpnChecked(bool pushButtonNewServerSettingsOpenvpnChecked)
{
    if (m_pushButtonNewServerSettingsOpenvpnChecked != pushButtonNewServerSettingsOpenvpnChecked) {
        m_pushButtonNewServerSettingsOpenvpnChecked = pushButtonNewServerSettingsOpenvpnChecked;
        emit pushButtonNewServerSettingsOpenvpnCheckedChanged();
    }
}

QString UiLogic::getLineEditNewServerCloakPortText() const
{
    return m_lineEditNewServerCloakPortText;
}

void UiLogic::setLineEditNewServerCloakPortText(const QString &lineEditNewServerCloakPortText)
{
    if (m_lineEditNewServerCloakPortText != lineEditNewServerCloakPortText) {
        m_lineEditNewServerCloakPortText = lineEditNewServerCloakPortText;
        emit lineEditNewServerCloakPortTextChanged();
    }
}

bool UiLogic::getPushButtonNewServerSettingsCloakChecked() const
{
    return m_pushButtonNewServerSettingsCloakChecked;
}

void UiLogic::setPushButtonNewServerSettingsCloakChecked(bool pushButtonNewServerSettingsCloakChecked)
{
    if (m_pushButtonNewServerSettingsCloakChecked != pushButtonNewServerSettingsCloakChecked) {
        m_pushButtonNewServerSettingsCloakChecked = pushButtonNewServerSettingsCloakChecked;
        emit pushButtonNewServerSettingsCloakCheckedChanged();
    }
}

double UiLogic::getProgressBarNewServerConfiguringValue() const
{
    return m_progressBarNewServerConfiguringValue;
}

void UiLogic::setProgressBarNewServerConfiguringValue(double progressBarNewServerConfiguringValue)
{
    if (m_progressBarNewServerConfiguringValue != progressBarNewServerConfiguringValue) {
        m_progressBarNewServerConfiguringValue = progressBarNewServerConfiguringValue;
        emit progressBarNewServerConfiguringValueChanged();
    }
}

QString UiLogic::getLineEditSetupWizardHighWebsiteMaskingText() const
{
    return m_lineEditSetupWizardHighWebsiteMaskingText;
}

void UiLogic::setLineEditSetupWizardHighWebsiteMaskingText(const QString &lineEditSetupWizardHighWebsiteMaskingText)
{
    if (m_lineEditSetupWizardHighWebsiteMaskingText != lineEditSetupWizardHighWebsiteMaskingText) {
        m_lineEditSetupWizardHighWebsiteMaskingText = lineEditSetupWizardHighWebsiteMaskingText;
        emit lineEditSetupWizardHighWebsiteMaskingTextChanged();
    }
}
QString UiLogic::getLineEditStartExistingCodeText() const
{
    return m_lineEditStartExistingCodeText;
}

void UiLogic::setLineEditStartExistingCodeText(const QString &lineEditStartExistingCodeText)
{
    if (m_lineEditStartExistingCodeText != lineEditStartExistingCodeText) {
        m_lineEditStartExistingCodeText = lineEditStartExistingCodeText;
        emit lineEditStartExistingCodeTextChanged();
    }
}

QString UiLogic::getTextEditNewServerSshKeyText() const
{
    return m_textEditNewServerSshKeyText;
}

void UiLogic::setTextEditNewServerSshKeyText(const QString &textEditNewServerSshKeyText)
{
    if (m_textEditNewServerSshKeyText != textEditNewServerSshKeyText) {
        m_textEditNewServerSshKeyText = textEditNewServerSshKeyText;
        emit textEditNewServerSshKeyTextChanged();
    }
}

QString UiLogic::getLineEditNewServerIpText() const
{
    return m_lineEditNewServerIpText;
}

void UiLogic::setLineEditNewServerIpText(const QString &lineEditNewServerIpText)
{
    if (m_lineEditNewServerIpText != lineEditNewServerIpText) {
        m_lineEditNewServerIpText = lineEditNewServerIpText;
        emit lineEditNewServerIpTextChanged();
    }
}

QString UiLogic::getLineEditNewServerPasswordText() const
{
    return m_lineEditNewServerPasswordText;
}

void UiLogic::setLineEditNewServerPasswordText(const QString &lineEditNewServerPasswordText)
{
    if (m_lineEditNewServerPasswordText != lineEditNewServerPasswordText) {
        m_lineEditNewServerPasswordText = lineEditNewServerPasswordText;
        emit lineEditNewServerPasswordTextChanged();
    }
}

QString UiLogic::getLineEditNewServerLoginText() const
{
    return m_lineEditNewServerLoginText;
}

void UiLogic::setLineEditNewServerLoginText(const QString &lineEditNewServerLoginText)
{
    if (m_lineEditNewServerLoginText != lineEditNewServerLoginText) {
        m_lineEditNewServerLoginText = lineEditNewServerLoginText;
        emit lineEditNewServerLoginTextChanged();
    }
}

bool UiLogic::getLabelNewServerWaitInfoVisible() const
{
    return m_labelNewServerWaitInfoVisible;
}

void UiLogic::setLabelNewServerWaitInfoVisible(bool labelNewServerWaitInfoVisible)
{
    if (m_labelNewServerWaitInfoVisible != labelNewServerWaitInfoVisible) {
        m_labelNewServerWaitInfoVisible = labelNewServerWaitInfoVisible;
        emit labelNewServerWaitInfoVisibleChanged();
    }
}

QString UiLogic::getLabelNewServerWaitInfoText() const
{
    return m_labelNewServerWaitInfoText;
}

void UiLogic::setLabelNewServerWaitInfoText(const QString &labelNewServerWaitInfoText)
{
    if (m_labelNewServerWaitInfoText != labelNewServerWaitInfoText) {
        m_labelNewServerWaitInfoText = labelNewServerWaitInfoText;
        emit labelNewServerWaitInfoTextChanged();
    }
}

double UiLogic::getProgressBarNewServerConnectionMinimum() const
{
    return m_progressBarNewServerConnectionMinimum;
}

void UiLogic::setProgressBarNewServerConnectionMinimum(double progressBarNewServerConnectionMinimum)
{
    if (m_progressBarNewServerConnectionMinimum != progressBarNewServerConnectionMinimum) {
        m_progressBarNewServerConnectionMinimum = progressBarNewServerConnectionMinimum;
        emit progressBarNewServerConnectionMinimumChanged();
    }
}

double UiLogic::getProgressBarNewServerConnectionMaximum() const
{
    return m_progressBarNewServerConnectionMaximum;
}

void UiLogic::setProgressBarNewServerConnectionMaximum(double progressBarNewServerConnectionMaximum)
{
    if (m_progressBarNewServerConnectionMaximum != progressBarNewServerConnectionMaximum) {
        m_progressBarNewServerConnectionMaximum = progressBarNewServerConnectionMaximum;
        emit progressBarNewServerConnectionMaximumChanged();
    }
}

bool UiLogic::getPushButtonBackFromStartVisible() const
{
    return m_pushButtonBackFromStartVisible;
}

void UiLogic::setPushButtonBackFromStartVisible(bool pushButtonBackFromStartVisible)
{
    if (m_pushButtonBackFromStartVisible != pushButtonBackFromStartVisible) {
        m_pushButtonBackFromStartVisible = pushButtonBackFromStartVisible;
        emit pushButtonBackFromStartVisibleChanged();
    }
}

bool UiLogic::getPushButtonNewServerConnectVisible() const
{
    return m_pushButtonNewServerConnectVisible;
}

void UiLogic::setPushButtonNewServerConnectVisible(bool pushButtonNewServerConnectVisible)
{
    if (m_pushButtonNewServerConnectVisible != pushButtonNewServerConnectVisible) {
        m_pushButtonNewServerConnectVisible = pushButtonNewServerConnectVisible;
        emit pushButtonNewServerConnectVisibleChanged();
    }
}

bool UiLogic::getRadioButtonVpnModeAllSitesChecked() const
{
    return m_radioButtonVpnModeAllSitesChecked;
}

bool UiLogic::getRadioButtonVpnModeForwardSitesChecked() const
{
    return m_radioButtonVpnModeForwardSitesChecked;
}

bool UiLogic::getRadioButtonVpnModeExceptSitesChecked() const
{
    return m_radioButtonVpnModeExceptSitesChecked;
}

void UiLogic::setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked)
{
    if (m_radioButtonVpnModeAllSitesChecked != radioButtonVpnModeAllSitesChecked) {
        m_radioButtonVpnModeAllSitesChecked = radioButtonVpnModeAllSitesChecked;
        emit radioButtonVpnModeAllSitesCheckedChanged();
    }
}

void UiLogic::setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked)
{
    if (m_radioButtonVpnModeForwardSitesChecked != radioButtonVpnModeForwardSitesChecked) {
        m_radioButtonVpnModeForwardSitesChecked = radioButtonVpnModeForwardSitesChecked;
        emit radioButtonVpnModeForwardSitesCheckedChanged();
    }
}

void UiLogic::setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked)
{
    if (m_radioButtonVpnModeExceptSitesChecked != radioButtonVpnModeExceptSitesChecked) {
        m_radioButtonVpnModeExceptSitesChecked = radioButtonVpnModeExceptSitesChecked;
        emit radioButtonVpnModeExceptSitesCheckedChanged();
    }
}







bool UiLogic::getLabelServerSettingsWaitInfoVisible() const
{
    return m_labelServerSettingsWaitInfoVisible;
}

void UiLogic::setLabelServerSettingsWaitInfoVisible(bool labelServerSettingsWaitInfoVisible)
{
    if (m_labelServerSettingsWaitInfoVisible != labelServerSettingsWaitInfoVisible) {
        m_labelServerSettingsWaitInfoVisible = labelServerSettingsWaitInfoVisible;
        emit labelServerSettingsWaitInfoVisibleChanged();
    }
}

QString UiLogic::getLabelServerSettingsWaitInfoText() const
{
    return m_labelServerSettingsWaitInfoText;
}

void UiLogic::setLabelServerSettingsWaitInfoText(const QString &labelServerSettingsWaitInfoText)
{
    if (m_labelServerSettingsWaitInfoText != labelServerSettingsWaitInfoText) {
        m_labelServerSettingsWaitInfoText = labelServerSettingsWaitInfoText;
        emit labelServerSettingsWaitInfoTextChanged();
    }
}

bool UiLogic::getPushButtonServerSettingsClearVisible() const
{
    return m_pushButtonServerSettingsClearVisible;
}

void UiLogic::setPushButtonServerSettingsClearVisible(bool pushButtonServerSettingsClearVisible)
{
    if (m_pushButtonServerSettingsClearVisible != pushButtonServerSettingsClearVisible) {
        m_pushButtonServerSettingsClearVisible = pushButtonServerSettingsClearVisible;
        emit pushButtonServerSettingsClearVisibleChanged();
    }
}

bool UiLogic::getPushButtonServerSettingsClearClientCacheVisible() const
{
    return m_pushButtonServerSettingsClearClientCacheVisible;
}

void UiLogic::setPushButtonServerSettingsClearClientCacheVisible(bool pushButtonServerSettingsClearClientCacheVisible)
{
    if (m_pushButtonServerSettingsClearClientCacheVisible != pushButtonServerSettingsClearClientCacheVisible) {
        m_pushButtonServerSettingsClearClientCacheVisible = pushButtonServerSettingsClearClientCacheVisible;
        emit pushButtonServerSettingsClearClientCacheVisibleChanged();
    }
}

bool UiLogic::getPushButtonServerSettingsShareFullVisible() const
{
    return m_pushButtonServerSettingsShareFullVisible;
}

void UiLogic::setPushButtonServerSettingsShareFullVisible(bool pushButtonServerSettingsShareFullVisible)
{
    if (m_pushButtonServerSettingsShareFullVisible != pushButtonServerSettingsShareFullVisible) {
        m_pushButtonServerSettingsShareFullVisible = pushButtonServerSettingsShareFullVisible;
        emit pushButtonServerSettingsShareFullVisibleChanged();
    }
}

QString UiLogic::getLabelServerSettingsServerText() const
{
    return m_labelServerSettingsServerText;
}

void UiLogic::setLabelServerSettingsServerText(const QString &labelServerSettingsServerText)
{
    if (m_labelServerSettingsServerText != labelServerSettingsServerText) {
        m_labelServerSettingsServerText = labelServerSettingsServerText;
        emit labelServerSettingsServerTextChanged();
    }
}

QString UiLogic::getLineEditServerSettingsDescriptionText() const
{
    return m_lineEditServerSettingsDescriptionText;
}

void UiLogic::setLineEditServerSettingsDescriptionText(const QString &lineEditServerSettingsDescriptionText)
{
    if (m_lineEditServerSettingsDescriptionText != lineEditServerSettingsDescriptionText) {
        m_lineEditServerSettingsDescriptionText = lineEditServerSettingsDescriptionText;
        emit lineEditServerSettingsDescriptionTextChanged();
    }
}

QString UiLogic::getLabelServerSettingsCurrentVpnProtocolText() const
{
    return m_labelServerSettingsCurrentVpnProtocolText;
}

void UiLogic::setLabelServerSettingsCurrentVpnProtocolText(const QString &labelServerSettingsCurrentVpnProtocolText)
{
    if (m_labelServerSettingsCurrentVpnProtocolText != labelServerSettingsCurrentVpnProtocolText) {
        m_labelServerSettingsCurrentVpnProtocolText = labelServerSettingsCurrentVpnProtocolText;
        emit labelServerSettingsCurrentVpnProtocolTextChanged();
    }
}
QString UiLogic::getComboBoxProtoCloakCipherText() const
{
    return m_comboBoxProtoCloakCipherText;
}

void UiLogic::setComboBoxProtoCloakCipherText(const QString &comboBoxProtoCloakCipherText)
{
    if (m_comboBoxProtoCloakCipherText != comboBoxProtoCloakCipherText) {
        m_comboBoxProtoCloakCipherText = comboBoxProtoCloakCipherText;
        emit comboBoxProtoCloakCipherTextChanged();
    }
}

QString UiLogic::getLineEditProtoCloakPortText() const
{
    return m_lineEditProtoCloakPortText;
}

void UiLogic::setLineEditProtoCloakPortText(const QString &lineEditProtoCloakPortText)
{
    if (m_lineEditProtoCloakPortText != lineEditProtoCloakPortText) {
        m_lineEditProtoCloakPortText = lineEditProtoCloakPortText;
        emit lineEditProtoCloakPortTextChanged();
    }
}

QString UiLogic::getLineEditProtoCloakSiteText() const
{
    return m_lineEditProtoCloakSiteText;
}

void UiLogic::setLineEditProtoCloakSiteText(const QString &lineEditProtoCloakSiteText)
{
    if (m_lineEditProtoCloakSiteText != lineEditProtoCloakSiteText) {
        m_lineEditProtoCloakSiteText = lineEditProtoCloakSiteText;
        emit lineEditProtoCloakSiteTextChanged();
    }
}

int UiLogic::getCurrentPageValue() const
{
    return m_currentPageValue;
}

void UiLogic::setCurrentPageValue(int currentPageValue)
{
    if (m_currentPageValue != currentPageValue) {
        m_currentPageValue = currentPageValue;
        emit currentPageValueChanged();
    }
}

QString UiLogic::getTrayIconUrl() const
{
    return m_trayIconUrl;
}

void UiLogic::setTrayIconUrl(const QString &trayIconUrl)
{
    if (m_trayIconUrl != trayIconUrl) {
        m_trayIconUrl = trayIconUrl;
        emit trayIconUrlChanged();
    }
}

bool UiLogic::getTrayActionDisconnectEnabled() const
{
    return m_trayActionDisconnectEnabled;
}

void UiLogic::setTrayActionDisconnectEnabled(bool trayActionDisconnectEnabled)
{
    if (m_trayActionDisconnectEnabled != trayActionDisconnectEnabled) {
        m_trayActionDisconnectEnabled = trayActionDisconnectEnabled;
        emit trayActionDisconnectEnabledChanged();
    }
}

bool UiLogic::getTrayActionConnectEnabled() const
{
    return m_trayActionConnectEnabled;
}

void UiLogic::setTrayActionConnectEnabled(bool trayActionConnectEnabled)
{
    if (m_trayActionConnectEnabled != trayActionConnectEnabled) {
        m_trayActionConnectEnabled = trayActionConnectEnabled;
        emit trayActionConnectEnabledChanged();
    }
}

bool UiLogic::getCheckBoxNewServerCloakChecked() const
{
    return m_checkBoxNewServerCloakChecked;
}

void UiLogic::setCheckBoxNewServerCloakChecked(bool checkBoxNewServerCloakChecked)
{
    if (m_checkBoxNewServerCloakChecked != checkBoxNewServerCloakChecked) {
        m_checkBoxNewServerCloakChecked = checkBoxNewServerCloakChecked;
        emit checkBoxNewServerCloakCheckedChanged();
    }
}

bool UiLogic::getCheckBoxNewServerSsChecked() const
{
    return m_checkBoxNewServerSsChecked;
}

void UiLogic::setCheckBoxNewServerSsChecked(bool checkBoxNewServerSsChecked)
{
    if (m_checkBoxNewServerSsChecked != checkBoxNewServerSsChecked) {
        m_checkBoxNewServerSsChecked = checkBoxNewServerSsChecked;
        emit checkBoxNewServerSsCheckedChanged();
    }
}

bool UiLogic::getCheckBoxNewServerOpenvpnChecked() const
{
    return m_checkBoxNewServerOpenvpnChecked;
}

void UiLogic::setCheckBoxNewServerOpenvpnChecked(bool checkBoxNewServerOpenvpnChecked)
{
    if (m_checkBoxNewServerOpenvpnChecked != checkBoxNewServerOpenvpnChecked) {
        m_checkBoxNewServerOpenvpnChecked = checkBoxNewServerOpenvpnChecked;
        emit checkBoxNewServerOpenvpnCheckedChanged();
    }
}

QString UiLogic::getComboBoxProtoShadowsocksCipherText() const
{
    return m_comboBoxProtoShadowsocksCipherText;
}

void UiLogic::setComboBoxProtoShadowsocksCipherText(const QString &comboBoxProtoShadowsocksCipherText)
{
    if (m_comboBoxProtoShadowsocksCipherText != comboBoxProtoShadowsocksCipherText) {
        m_comboBoxProtoShadowsocksCipherText = comboBoxProtoShadowsocksCipherText;
        emit comboBoxProtoShadowsocksCipherTextChanged();
    }
}

QString UiLogic::getLineEditProtoShadowsocksPortText() const
{
    return m_lineEditProtoShadowsocksPortText;
}

void UiLogic::setLineEditProtoShadowsocksPortText(const QString &lineEditProtoShadowsocksPortText)
{
    if (m_lineEditProtoShadowsocksPortText != lineEditProtoShadowsocksPortText) {
        m_lineEditProtoShadowsocksPortText = lineEditProtoShadowsocksPortText;
        emit lineEditProtoShadowsocksPortTextChanged();
    }
}

QString UiLogic::getLineEditProtoOpenvpnSubnetText() const
{
    return m_lineEditProtoOpenvpnSubnetText;
}

void UiLogic::setLineEditProtoOpenvpnSubnetText(const QString &lineEditProtoOpenvpnSubnetText)
{
    if (m_lineEditProtoOpenvpnSubnetText != lineEditProtoOpenvpnSubnetText) {
        m_lineEditProtoOpenvpnSubnetText = lineEditProtoOpenvpnSubnetText;
        emit lineEditProtoOpenvpnSubnetTextChanged();
    }
}

bool UiLogic::getRadioButtonProtoOpenvpnUdpChecked() const
{
    return m_radioButtonProtoOpenvpnUdpChecked;
}

void UiLogic::setRadioButtonProtoOpenvpnUdpChecked(bool radioButtonProtoOpenvpnUdpChecked)
{
    if (m_radioButtonProtoOpenvpnUdpChecked != radioButtonProtoOpenvpnUdpChecked) {
        m_radioButtonProtoOpenvpnUdpChecked = radioButtonProtoOpenvpnUdpChecked;
        emit radioButtonProtoOpenvpnUdpCheckedChanged();
    }
}

bool UiLogic::getCheckBoxProtoOpenvpnAutoEncryptionChecked() const
{
    return m_checkBoxProtoOpenvpnAutoEncryptionChecked;
}

void UiLogic::setCheckBoxProtoOpenvpnAutoEncryptionChecked(bool checkBoxProtoOpenvpnAutoEncryptionChecked)
{
    if (m_checkBoxProtoOpenvpnAutoEncryptionChecked != checkBoxProtoOpenvpnAutoEncryptionChecked) {
        m_checkBoxProtoOpenvpnAutoEncryptionChecked = checkBoxProtoOpenvpnAutoEncryptionChecked;
        emit checkBoxProtoOpenvpnAutoEncryptionCheckedChanged();
    }
}

QString UiLogic::getComboBoxProtoOpenvpnCipherText() const
{
    return m_comboBoxProtoOpenvpnCipherText;
}

void UiLogic::setComboBoxProtoOpenvpnCipherText(const QString &comboBoxProtoOpenvpnCipherText)
{
    if (m_comboBoxProtoOpenvpnCipherText != comboBoxProtoOpenvpnCipherText) {
        m_comboBoxProtoOpenvpnCipherText = comboBoxProtoOpenvpnCipherText;
        emit comboBoxProtoOpenvpnCipherTextChanged();
    }
}

QString UiLogic::getComboBoxProtoOpenvpnHashText() const
{
    return m_comboBoxProtoOpenvpnHashText;
}

void UiLogic::setComboBoxProtoOpenvpnHashText(const QString &comboBoxProtoOpenvpnHashText)
{
    if (m_comboBoxProtoOpenvpnHashText != comboBoxProtoOpenvpnHashText) {
        m_comboBoxProtoOpenvpnHashText = comboBoxProtoOpenvpnHashText;
        emit comboBoxProtoOpenvpnHashTextChanged();
    }
}

bool UiLogic::getCheckBoxProtoOpenvpnBlockDnsChecked() const
{
    return m_checkBoxProtoOpenvpnBlockDnsChecked;
}

void UiLogic::setCheckBoxProtoOpenvpnBlockDnsChecked(bool checkBoxProtoOpenvpnBlockDnsChecked)
{
    if (m_checkBoxProtoOpenvpnBlockDnsChecked != checkBoxProtoOpenvpnBlockDnsChecked) {
        m_checkBoxProtoOpenvpnBlockDnsChecked = checkBoxProtoOpenvpnBlockDnsChecked;
        emit checkBoxProtoOpenvpnBlockDnsCheckedChanged();
    }
}

QString UiLogic::getLineEditProtoOpenvpnPortText() const
{
    return m_lineEditProtoOpenvpnPortText;
}

void UiLogic::setLineEditProtoOpenvpnPortText(const QString &lineEditProtoOpenvpnPortText)
{
    if (m_lineEditProtoOpenvpnPortText != lineEditProtoOpenvpnPortText) {
        m_lineEditProtoOpenvpnPortText = lineEditProtoOpenvpnPortText;
        emit lineEditProtoOpenvpnPortTextChanged();
    }
}

bool UiLogic::getCheckBoxProtoOpenvpnTlsAuthChecked() const
{
    return m_checkBoxProtoOpenvpnTlsAuthChecked;
}

void UiLogic::setCheckBoxProtoOpenvpnTlsAuthChecked(bool checkBoxProtoOpenvpnTlsAuthChecked)
{
    if (m_checkBoxProtoOpenvpnTlsAuthChecked != checkBoxProtoOpenvpnTlsAuthChecked) {
        m_checkBoxProtoOpenvpnTlsAuthChecked = checkBoxProtoOpenvpnTlsAuthChecked;
        emit checkBoxProtoOpenvpnTlsAuthCheckedChanged();
    }
}

bool UiLogic::getRadioButtonSetupWizardHighChecked() const
{
    return m_radioButtonSetupWizardHighChecked;
}

void UiLogic::setRadioButtonSetupWizardHighChecked(bool radioButtonSetupWizardHighChecked)
{
    if (m_radioButtonSetupWizardHighChecked != radioButtonSetupWizardHighChecked) {
        m_radioButtonSetupWizardHighChecked = radioButtonSetupWizardHighChecked;
        emit radioButtonSetupWizardHighCheckedChanged();
    }
}

bool UiLogic::getRadioButtonSetupWizardLowChecked() const
{
    return m_radioButtonSetupWizardLowChecked;
}

void UiLogic::setRadioButtonSetupWizardLowChecked(bool radioButtonSetupWizardLowChecked)
{
    if (m_radioButtonSetupWizardLowChecked != radioButtonSetupWizardLowChecked) {
        m_radioButtonSetupWizardLowChecked = radioButtonSetupWizardLowChecked;
        emit radioButtonSetupWizardLowCheckedChanged();
    }
}

bool UiLogic::getCheckBoxSetupWizardVpnModeChecked() const
{
    return m_checkBoxSetupWizardVpnModeChecked;
}

void UiLogic::setCheckBoxSetupWizardVpnModeChecked(bool checkBoxSetupWizardVpnModeChecked)
{
    if (m_checkBoxSetupWizardVpnModeChecked != checkBoxSetupWizardVpnModeChecked) {
        m_checkBoxSetupWizardVpnModeChecked = checkBoxSetupWizardVpnModeChecked;
        emit checkBoxSetupWizardVpnModeCheckedChanged();
    }
}



bool UiLogic::getPushButtonConnectChecked() const
{
    return m_pushButtonConnectChecked;
}

void UiLogic::setPushButtonConnectChecked(bool pushButtonConnectChecked)
{
    if (m_pushButtonConnectChecked != pushButtonConnectChecked) {
        m_pushButtonConnectChecked = pushButtonConnectChecked;
        emit pushButtonConnectCheckedChanged();
    }
}

bool UiLogic::getPushButtonVpnAddSiteEnabled() const
{
    return m_pushButtonVpnAddSiteEnabled;
}

void UiLogic::setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled)
{
    if (m_pushButtonVpnAddSiteEnabled != pushButtonVpnAddSiteEnabled) {
        m_pushButtonVpnAddSiteEnabled = pushButtonVpnAddSiteEnabled;
        emit pushButtonVpnAddSiteEnabledChanged();
    }
}

bool UiLogic::getWidgetProtoCloakEnabled() const
{
    return m_widgetProtoCloakEnabled;
}

void UiLogic::setWidgetProtoCloakEnabled(bool widgetProtoCloakEnabled)
{
    if (m_widgetProtoCloakEnabled != widgetProtoCloakEnabled) {
        m_widgetProtoCloakEnabled = widgetProtoCloakEnabled;
        emit widgetProtoCloakEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakSaveVisible() const
{
    return m_pushButtonProtoCloakSaveVisible;
}

void UiLogic::setPushButtonProtoCloakSaveVisible(bool pushButtonProtoCloakSaveVisible)
{
    if (m_pushButtonProtoCloakSaveVisible != pushButtonProtoCloakSaveVisible) {
        m_pushButtonProtoCloakSaveVisible = pushButtonProtoCloakSaveVisible;
        emit pushButtonProtoCloakSaveVisibleChanged();
    }
}

bool UiLogic::getProgressBarProtoCloakResetVisible() const
{
    return m_progressBarProtoCloakResetVisible;
}

void UiLogic::setProgressBarProtoCloakResetVisible(bool progressBarProtoCloakResetVisible)
{
    if (m_progressBarProtoCloakResetVisible != progressBarProtoCloakResetVisible) {
        m_progressBarProtoCloakResetVisible = progressBarProtoCloakResetVisible;
        emit progressBarProtoCloakResetVisibleChanged();
    }
}

bool UiLogic::getLineEditProtoCloakPortEnabled() const
{
    return m_lineEditProtoCloakPortEnabled;
}

void UiLogic::setLineEditProtoCloakPortEnabled(bool lineEditProtoCloakPortEnabled)
{
    if (m_lineEditProtoCloakPortEnabled != lineEditProtoCloakPortEnabled) {
        m_lineEditProtoCloakPortEnabled = lineEditProtoCloakPortEnabled;
        emit lineEditProtoCloakPortEnabledChanged();
    }
}
bool UiLogic::getWidgetProtoSsEnabled() const
{
    return m_widgetProtoSsEnabled;
}

void UiLogic::setWidgetProtoSsEnabled(bool widgetProtoSsEnabled)
{
    if (m_widgetProtoSsEnabled != widgetProtoSsEnabled) {
        m_widgetProtoSsEnabled = widgetProtoSsEnabled;
        emit widgetProtoSsEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoShadowsocksSaveVisible() const
{
    return m_pushButtonProtoShadowsocksSaveVisible;
}

void UiLogic::setPushButtonProtoShadowsocksSaveVisible(bool pushButtonProtoShadowsocksSaveVisible)
{
    if (m_pushButtonProtoShadowsocksSaveVisible != pushButtonProtoShadowsocksSaveVisible) {
        m_pushButtonProtoShadowsocksSaveVisible = pushButtonProtoShadowsocksSaveVisible;
        emit pushButtonProtoShadowsocksSaveVisibleChanged();
    }
}

bool UiLogic::getProgressBarProtoShadowsocksResetVisible() const
{
    return m_progressBarProtoShadowsocksResetVisible;
}

void UiLogic::setProgressBarProtoShadowsocksResetVisible(bool progressBarProtoShadowsocksResetVisible)
{
    if (m_progressBarProtoShadowsocksResetVisible != progressBarProtoShadowsocksResetVisible) {
        m_progressBarProtoShadowsocksResetVisible = progressBarProtoShadowsocksResetVisible;
        emit progressBarProtoShadowsocksResetVisibleChanged();
    }
}

bool UiLogic::getLineEditProtoShadowsocksPortEnabled() const
{
    return m_lineEditProtoShadowsocksPortEnabled;
}

void UiLogic::setLineEditProtoShadowsocksPortEnabled(bool lineEditProtoShadowsocksPortEnabled)
{
    if (m_lineEditProtoShadowsocksPortEnabled != lineEditProtoShadowsocksPortEnabled) {
        m_lineEditProtoShadowsocksPortEnabled = lineEditProtoShadowsocksPortEnabled;
        emit lineEditProtoShadowsocksPortEnabledChanged();
    }
}

bool UiLogic::getWidgetProtoOpenvpnEnabled() const
{
    return m_widgetProtoOpenvpnEnabled;
}

void UiLogic::setWidgetProtoOpenvpnEnabled(bool widgetProtoOpenvpnEnabled)
{
    if (m_widgetProtoOpenvpnEnabled != widgetProtoOpenvpnEnabled) {
        m_widgetProtoOpenvpnEnabled = widgetProtoOpenvpnEnabled;
        emit widgetProtoOpenvpnEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnSaveVisible() const
{
    return m_pushButtonProtoOpenvpnSaveVisible;
}

void UiLogic::setPushButtonProtoOpenvpnSaveVisible(bool pushButtonProtoOpenvpnSaveVisible)
{
    if (m_pushButtonProtoOpenvpnSaveVisible != pushButtonProtoOpenvpnSaveVisible) {
        m_pushButtonProtoOpenvpnSaveVisible = pushButtonProtoOpenvpnSaveVisible;
        emit pushButtonProtoOpenvpnSaveVisibleChanged();
    }
}

bool UiLogic::getProgressBarProtoOpenvpnResetVisible() const
{
    return m_progressBarProtoOpenvpnResetVisible;
}

void UiLogic::setProgressBarProtoOpenvpnResetVisible(bool progressBarProtoOpenvpnResetVisible)
{
    if (m_progressBarProtoOpenvpnResetVisible != progressBarProtoOpenvpnResetVisible) {
        m_progressBarProtoOpenvpnResetVisible = progressBarProtoOpenvpnResetVisible;
        emit progressBarProtoOpenvpnResetVisibleChanged();
    }
}

bool UiLogic::getRadioButtonProtoOpenvpnUdpEnabled() const
{
    return m_radioButtonProtoOpenvpnUdpEnabled;
}

void UiLogic::setRadioButtonProtoOpenvpnUdpEnabled(bool radioButtonProtoOpenvpnUdpEnabled)
{
    if (m_radioButtonProtoOpenvpnUdpEnabled != radioButtonProtoOpenvpnUdpEnabled) {
        m_radioButtonProtoOpenvpnUdpEnabled = radioButtonProtoOpenvpnUdpEnabled;
        emit radioButtonProtoOpenvpnUdpEnabledChanged();
    }
}

bool UiLogic::getRadioButtonProtoOpenvpnTcpEnabled() const
{
    return m_radioButtonProtoOpenvpnTcpEnabled;
}

void UiLogic::setRadioButtonProtoOpenvpnTcpEnabled(bool radioButtonProtoOpenvpnTcpEnabled)
{
    if (m_radioButtonProtoOpenvpnTcpEnabled != radioButtonProtoOpenvpnTcpEnabled) {
        m_radioButtonProtoOpenvpnTcpEnabled = radioButtonProtoOpenvpnTcpEnabled;
        emit radioButtonProtoOpenvpnTcpEnabledChanged();
    }
}

bool UiLogic::getRadioButtonProtoOpenvpnTcpChecked() const
{
    return m_radioButtonProtoOpenvpnTcpChecked;
}

void UiLogic::setRadioButtonProtoOpenvpnTcpChecked(bool radioButtonProtoOpenvpnTcpChecked)
{
    if (m_radioButtonProtoOpenvpnTcpChecked != radioButtonProtoOpenvpnTcpChecked) {
        m_radioButtonProtoOpenvpnTcpChecked = radioButtonProtoOpenvpnTcpChecked;
        emit radioButtonProtoOpenvpnTcpCheckedChanged();
    }
}

bool UiLogic::getLineEditProtoOpenvpnPortEnabled() const
{
    return m_lineEditProtoOpenvpnPortEnabled;
}

void UiLogic::setLineEditProtoOpenvpnPortEnabled(bool lineEditProtoOpenvpnPortEnabled)
{
    if (m_lineEditProtoOpenvpnPortEnabled != lineEditProtoOpenvpnPortEnabled) {
        m_lineEditProtoOpenvpnPortEnabled = lineEditProtoOpenvpnPortEnabled;
        emit lineEditProtoOpenvpnPortEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoOpenvpnContInstallChecked;
}

void UiLogic::setPushButtonProtoOpenvpnContInstallChecked(bool pushButtonProtoOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoOpenvpnContInstallChecked != pushButtonProtoOpenvpnContInstallChecked) {
        m_pushButtonProtoOpenvpnContInstallChecked = pushButtonProtoOpenvpnContInstallChecked;
        emit pushButtonProtoOpenvpnContInstallCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoSsOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoSsOpenvpnContInstallChecked;
}

void UiLogic::setPushButtonProtoSsOpenvpnContInstallChecked(bool pushButtonProtoSsOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoSsOpenvpnContInstallChecked != pushButtonProtoSsOpenvpnContInstallChecked) {
        m_pushButtonProtoSsOpenvpnContInstallChecked = pushButtonProtoSsOpenvpnContInstallChecked;
        emit pushButtonProtoSsOpenvpnContInstallCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakOpenvpnContInstallChecked() const
{
    return m_pushButtonProtoCloakOpenvpnContInstallChecked;
}

void UiLogic::setPushButtonProtoCloakOpenvpnContInstallChecked(bool pushButtonProtoCloakOpenvpnContInstallChecked)
{
    if (m_pushButtonProtoCloakOpenvpnContInstallChecked != pushButtonProtoCloakOpenvpnContInstallChecked) {
        m_pushButtonProtoCloakOpenvpnContInstallChecked = pushButtonProtoCloakOpenvpnContInstallChecked;
        emit pushButtonProtoCloakOpenvpnContInstallCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoWireguardContInstallChecked() const
{
    return m_pushButtonProtoWireguardContInstallChecked;
}

void UiLogic::setPushButtonProtoWireguardContInstallChecked(bool pushButtonProtoWireguardContInstallChecked)
{
    if (m_pushButtonProtoWireguardContInstallChecked != pushButtonProtoWireguardContInstallChecked) {
        m_pushButtonProtoWireguardContInstallChecked = pushButtonProtoWireguardContInstallChecked;
        emit pushButtonProtoWireguardContInstallCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoOpenvpnContInstallEnabled;
}

void UiLogic::setPushButtonProtoOpenvpnContInstallEnabled(bool pushButtonProtoOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoOpenvpnContInstallEnabled != pushButtonProtoOpenvpnContInstallEnabled) {
        m_pushButtonProtoOpenvpnContInstallEnabled = pushButtonProtoOpenvpnContInstallEnabled;
        emit pushButtonProtoOpenvpnContInstallEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoSsOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoSsOpenvpnContInstallEnabled;
}

void UiLogic::setPushButtonProtoSsOpenvpnContInstallEnabled(bool pushButtonProtoSsOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoSsOpenvpnContInstallEnabled != pushButtonProtoSsOpenvpnContInstallEnabled) {
        m_pushButtonProtoSsOpenvpnContInstallEnabled = pushButtonProtoSsOpenvpnContInstallEnabled;
        emit pushButtonProtoSsOpenvpnContInstallEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakOpenvpnContInstallEnabled() const
{
    return m_pushButtonProtoCloakOpenvpnContInstallEnabled;
}

void UiLogic::setPushButtonProtoCloakOpenvpnContInstallEnabled(bool pushButtonProtoCloakOpenvpnContInstallEnabled)
{
    if (m_pushButtonProtoCloakOpenvpnContInstallEnabled != pushButtonProtoCloakOpenvpnContInstallEnabled) {
        m_pushButtonProtoCloakOpenvpnContInstallEnabled = pushButtonProtoCloakOpenvpnContInstallEnabled;
        emit pushButtonProtoCloakOpenvpnContInstallEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoWireguardContInstallEnabled() const
{
    return m_pushButtonProtoWireguardContInstallEnabled;
}

void UiLogic::setPushButtonProtoWireguardContInstallEnabled(bool pushButtonProtoWireguardContInstallEnabled)
{
    if (m_pushButtonProtoWireguardContInstallEnabled != pushButtonProtoWireguardContInstallEnabled) {
        m_pushButtonProtoWireguardContInstallEnabled = pushButtonProtoWireguardContInstallEnabled;
        emit pushButtonProtoWireguardContInstallEnabledChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoOpenvpnContDefaultChecked;
}

void UiLogic::setPushButtonProtoOpenvpnContDefaultChecked(bool pushButtonProtoOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoOpenvpnContDefaultChecked != pushButtonProtoOpenvpnContDefaultChecked) {
        m_pushButtonProtoOpenvpnContDefaultChecked = pushButtonProtoOpenvpnContDefaultChecked;
        emit pushButtonProtoOpenvpnContDefaultCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoSsOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoSsOpenvpnContDefaultChecked;
}

void UiLogic::setPushButtonProtoSsOpenvpnContDefaultChecked(bool pushButtonProtoSsOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoSsOpenvpnContDefaultChecked != pushButtonProtoSsOpenvpnContDefaultChecked) {
        m_pushButtonProtoSsOpenvpnContDefaultChecked = pushButtonProtoSsOpenvpnContDefaultChecked;
        emit pushButtonProtoSsOpenvpnContDefaultCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakOpenvpnContDefaultChecked() const
{
    return m_pushButtonProtoCloakOpenvpnContDefaultChecked;
}

void UiLogic::setPushButtonProtoCloakOpenvpnContDefaultChecked(bool pushButtonProtoCloakOpenvpnContDefaultChecked)
{
    if (m_pushButtonProtoCloakOpenvpnContDefaultChecked != pushButtonProtoCloakOpenvpnContDefaultChecked) {
        m_pushButtonProtoCloakOpenvpnContDefaultChecked = pushButtonProtoCloakOpenvpnContDefaultChecked;
        emit pushButtonProtoCloakOpenvpnContDefaultCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoWireguardContDefaultChecked() const
{
    return m_pushButtonProtoWireguardContDefaultChecked;
}

void UiLogic::setPushButtonProtoWireguardContDefaultChecked(bool pushButtonProtoWireguardContDefaultChecked)
{
    if (m_pushButtonProtoWireguardContDefaultChecked != pushButtonProtoWireguardContDefaultChecked) {
        m_pushButtonProtoWireguardContDefaultChecked = pushButtonProtoWireguardContDefaultChecked;
        emit pushButtonProtoWireguardContDefaultCheckedChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoOpenvpnContDefaultVisible;
}

void UiLogic::setPushButtonProtoOpenvpnContDefaultVisible(bool pushButtonProtoOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoOpenvpnContDefaultVisible != pushButtonProtoOpenvpnContDefaultVisible) {
        m_pushButtonProtoOpenvpnContDefaultVisible = pushButtonProtoOpenvpnContDefaultVisible;
        emit pushButtonProtoOpenvpnContDefaultVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoSsOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoSsOpenvpnContDefaultVisible;
}

void UiLogic::setPushButtonProtoSsOpenvpnContDefaultVisible(bool pushButtonProtoSsOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoSsOpenvpnContDefaultVisible != pushButtonProtoSsOpenvpnContDefaultVisible) {
        m_pushButtonProtoSsOpenvpnContDefaultVisible = pushButtonProtoSsOpenvpnContDefaultVisible;
        emit pushButtonProtoSsOpenvpnContDefaultVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakOpenvpnContDefaultVisible() const
{
    return m_pushButtonProtoCloakOpenvpnContDefaultVisible;
}

void UiLogic::setPushButtonProtoCloakOpenvpnContDefaultVisible(bool pushButtonProtoCloakOpenvpnContDefaultVisible)
{
    if (m_pushButtonProtoCloakOpenvpnContDefaultVisible != pushButtonProtoCloakOpenvpnContDefaultVisible) {
        m_pushButtonProtoCloakOpenvpnContDefaultVisible = pushButtonProtoCloakOpenvpnContDefaultVisible;
        emit pushButtonProtoCloakOpenvpnContDefaultVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoWireguardContDefaultVisible() const
{
    return m_pushButtonProtoWireguardContDefaultVisible;
}

void UiLogic::setPushButtonProtoWireguardContDefaultVisible(bool pushButtonProtoWireguardContDefaultVisible)
{
    if (m_pushButtonProtoWireguardContDefaultVisible != pushButtonProtoWireguardContDefaultVisible) {
        m_pushButtonProtoWireguardContDefaultVisible = pushButtonProtoWireguardContDefaultVisible;
        emit pushButtonProtoWireguardContDefaultVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoOpenvpnContShareVisible() const
{
    return m_pushButtonProtoOpenvpnContShareVisible;
}

void UiLogic::setPushButtonProtoOpenvpnContShareVisible(bool pushButtonProtoOpenvpnContShareVisible)
{
    if (m_pushButtonProtoOpenvpnContShareVisible != pushButtonProtoOpenvpnContShareVisible) {
        m_pushButtonProtoOpenvpnContShareVisible = pushButtonProtoOpenvpnContShareVisible;
        emit pushButtonProtoOpenvpnContShareVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoSsOpenvpnContShareVisible() const
{
    return m_pushButtonProtoSsOpenvpnContShareVisible;
}

void UiLogic::setPushButtonProtoSsOpenvpnContShareVisible(bool pushButtonProtoSsOpenvpnContShareVisible)
{
    if (m_pushButtonProtoSsOpenvpnContShareVisible != pushButtonProtoSsOpenvpnContShareVisible) {
        m_pushButtonProtoSsOpenvpnContShareVisible = pushButtonProtoSsOpenvpnContShareVisible;
        emit pushButtonProtoSsOpenvpnContShareVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoCloakOpenvpnContShareVisible() const
{
    return m_pushButtonProtoCloakOpenvpnContShareVisible;
}

void UiLogic::setPushButtonProtoCloakOpenvpnContShareVisible(bool pushButtonProtoCloakOpenvpnContShareVisible)
{
    if (m_pushButtonProtoCloakOpenvpnContShareVisible != pushButtonProtoCloakOpenvpnContShareVisible) {
        m_pushButtonProtoCloakOpenvpnContShareVisible = pushButtonProtoCloakOpenvpnContShareVisible;
        emit pushButtonProtoCloakOpenvpnContShareVisibleChanged();
    }
}

bool UiLogic::getPushButtonProtoWireguardContShareVisible() const
{
    return m_pushButtonProtoWireguardContShareVisible;
}

void UiLogic::setPushButtonProtoWireguardContShareVisible(bool pushButtonProtoWireguardContShareVisible)
{
    if (m_pushButtonProtoWireguardContShareVisible != pushButtonProtoWireguardContShareVisible) {
        m_pushButtonProtoWireguardContShareVisible = pushButtonProtoWireguardContShareVisible;
        emit pushButtonProtoWireguardContShareVisibleChanged();
    }
}

bool UiLogic::getFrameOpenvpnSettingsVisible() const
{
    return m_frameOpenvpnSettingsVisible;
}

void UiLogic::setFrameOpenvpnSettingsVisible(bool frameOpenvpnSettingsVisible)
{
    if (m_frameOpenvpnSettingsVisible != frameOpenvpnSettingsVisible) {
        m_frameOpenvpnSettingsVisible = frameOpenvpnSettingsVisible;
        emit frameOpenvpnSettingsVisibleChanged();
    }
}

bool UiLogic::getFrameOpenvpnSsSettingsVisible() const
{
    return m_frameOpenvpnSsSettingsVisible;
}

void UiLogic::setFrameOpenvpnSsSettingsVisible(bool frameOpenvpnSsSettingsVisible)
{
    if (m_frameOpenvpnSsSettingsVisible != frameOpenvpnSsSettingsVisible) {
        m_frameOpenvpnSsSettingsVisible = frameOpenvpnSsSettingsVisible;
        emit frameOpenvpnSsSettingsVisibleChanged();
    }
}

bool UiLogic::getFrameOpenvpnSsCloakSettingsVisible() const
{
    return m_frameOpenvpnSsCloakSettingsVisible;
}

void UiLogic::setFrameOpenvpnSsCloakSettingsVisible(bool frameOpenvpnSsCloakSettingsVisible)
{
    if (m_frameOpenvpnSsCloakSettingsVisible != frameOpenvpnSsCloakSettingsVisible) {
        m_frameOpenvpnSsCloakSettingsVisible = frameOpenvpnSsCloakSettingsVisible;
        emit frameOpenvpnSsCloakSettingsVisibleChanged();
    }
}

bool UiLogic::getProgressBarProtocolsContainerReinstallVisible() const
{
    return m_progressBarProtocolsContainerReinstallVisible;
}

void UiLogic::setProgressBarProtocolsContainerReinstallVisible(bool progressBarProtocolsContainerReinstallVisible)
{
    if (m_progressBarProtocolsContainerReinstallVisible != progressBarProtocolsContainerReinstallVisible) {
        m_progressBarProtocolsContainerReinstallVisible = progressBarProtocolsContainerReinstallVisible;
        emit progressBarProtocolsContainerReinstallVisibleChanged();
    }
}

QString UiLogic::getLabelSpeedReceivedText() const
{
    return m_labelSpeedReceivedText;
}

void UiLogic::setLabelSpeedReceivedText(const QString &labelSpeedReceivedText)
{
    if (m_labelSpeedReceivedText != labelSpeedReceivedText) {
        m_labelSpeedReceivedText = labelSpeedReceivedText;
        emit labelSpeedReceivedTextChanged();
    }
}

QString UiLogic::getLabelSpeedSentText() const
{
    return m_labelSpeedSentText;
}

void UiLogic::setLabelSpeedSentText(const QString &labelSpeedSentText)
{
    if (m_labelSpeedSentText != labelSpeedSentText) {
        m_labelSpeedSentText = labelSpeedSentText;
        emit labelSpeedSentTextChanged();
    }
}

QString UiLogic::getLabelStateText() const
{
    return m_labelStateText;
}

void UiLogic::setLabelStateText(const QString &labelStateText)
{
    if (m_labelStateText != labelStateText) {
        m_labelStateText = labelStateText;
        emit labelStateTextChanged();
    }
}

bool UiLogic::getPushButtonConnectEnabled() const
{
    return m_pushButtonConnectEnabled;
}

void UiLogic::setPushButtonConnectEnabled(bool pushButtonConnectEnabled)
{
    if (m_pushButtonConnectEnabled != pushButtonConnectEnabled) {
        m_pushButtonConnectEnabled = pushButtonConnectEnabled;
        emit pushButtonConnectEnabledChanged();
    }
}

bool UiLogic::getWidgetVpnModeEnabled() const
{
    return m_widgetVpnModeEnabled;
}

void UiLogic::setWidgetVpnModeEnabled(bool widgetVpnModeEnabled)
{
    if (m_widgetVpnModeEnabled != widgetVpnModeEnabled) {
        m_widgetVpnModeEnabled = widgetVpnModeEnabled;
        emit widgetVpnModeEnabledChanged();
    }
}

QString UiLogic::getLabelErrorText() const
{
    return m_labelErrorText;
}

void UiLogic::setLabelErrorText(const QString &labelErrorText)
{
    if (m_labelErrorText != labelErrorText) {
        m_labelErrorText = labelErrorText;
        emit labelErrorTextChanged();
    }
}

bool UiLogic::getPushButtonNewServerConnectEnabled() const
{
    return m_pushButtonNewServerConnectEnabled;
}

void UiLogic::setPushButtonNewServerConnectEnabled(bool pushButtonNewServerConnectEnabled)
{
    if (m_pushButtonNewServerConnectEnabled != pushButtonNewServerConnectEnabled) {
        m_pushButtonNewServerConnectEnabled = pushButtonNewServerConnectEnabled;
        emit pushButtonNewServerConnectEnabledChanged();
    }
}

QString UiLogic::getPushButtonNewServerConnectText() const
{
    return m_pushButtonNewServerConnectText;
}

void UiLogic::setPushButtonNewServerConnectText(const QString &pushButtonNewServerConnectText)
{
    if (m_pushButtonNewServerConnectText != pushButtonNewServerConnectText) {
        m_pushButtonNewServerConnectText = pushButtonNewServerConnectText;
        emit pushButtonNewServerConnectTextChanged();
    }
}
QString UiLogic::getDialogConnectErrorText() const
{
    return m_dialogConnectErrorText;
}

void UiLogic::setDialogConnectErrorText(const QString &dialogConnectErrorText)
{
    if (m_dialogConnectErrorText != dialogConnectErrorText) {
        m_dialogConnectErrorText = dialogConnectErrorText;
        emit dialogConnectErrorTextChanged();
    }
}

bool UiLogic::getPageServerSettingsEnabled() const
{
    return m_pageServerSettingsEnabled;
}

void UiLogic::setPageServerSettingsEnabled(bool pageServerSettingsEnabled)
{
    if (m_pageServerSettingsEnabled != pageServerSettingsEnabled) {
        m_pageServerSettingsEnabled = pageServerSettingsEnabled;
        emit pageServerSettingsEnabledChanged();
    }
}

QString UiLogic::getPushButtonServerSettingsClearText() const
{
    return m_pushButtonServerSettingsClearText;
}

void UiLogic::setPushButtonServerSettingsClearText(const QString &pushButtonServerSettingsClearText)
{
    if (m_pushButtonServerSettingsClearText != pushButtonServerSettingsClearText) {
        m_pushButtonServerSettingsClearText = pushButtonServerSettingsClearText;
        emit pushButtonServerSettingsClearTextChanged();
    }
}

bool UiLogic::getPageShareAmneziaVisible() const
{
    return m_pageShareAmneziaVisible;
}

void UiLogic::setPageShareAmneziaVisible(bool pageShareAmneziaVisible)
{
    if (m_pageShareAmneziaVisible != pageShareAmneziaVisible) {
        m_pageShareAmneziaVisible = pageShareAmneziaVisible;
        emit pageShareAmneziaVisibleChanged();
    }
}

bool UiLogic::getPageShareOpenvpnVisible() const
{
    return m_pageShareOpenvpnVisible;
}

void UiLogic::setPageShareOpenvpnVisible(bool pageShareOpenvpnVisible)
{
    if (m_pageShareOpenvpnVisible != pageShareOpenvpnVisible) {
        m_pageShareOpenvpnVisible = pageShareOpenvpnVisible;
        emit pageShareOpenvpnVisibleChanged();
    }
}

bool UiLogic::getPageShareShadowsocksVisible() const
{
    return m_pageShareShadowsocksVisible;
}

void UiLogic::setPageShareShadowsocksVisible(bool pageShareShadowsocksVisible)
{
    if (m_pageShareShadowsocksVisible != pageShareShadowsocksVisible) {
        m_pageShareShadowsocksVisible = pageShareShadowsocksVisible;
        emit pageShareShadowsocksVisibleChanged();
    }
}

bool UiLogic::getPageShareCloakVisible() const
{
    return m_pageShareCloakVisible;
}

void UiLogic::setPageShareCloakVisible(bool pageShareCloakVisible)
{
    if (m_pageShareCloakVisible != pageShareCloakVisible) {
        m_pageShareCloakVisible = pageShareCloakVisible;
        emit pageShareCloakVisibleChanged();
    }
}

bool UiLogic::getPageShareFullAccessVisible() const
{
    return m_pageShareFullAccessVisible;
}

void UiLogic::setPageShareFullAccessVisible(bool pageShareFullAccessVisible)
{
    if (m_pageShareFullAccessVisible != pageShareFullAccessVisible) {
        m_pageShareFullAccessVisible = pageShareFullAccessVisible;
        emit pageShareFullAccessVisibleChanged();
    }
}

QString UiLogic::getTextEditShareOpenvpnCodeText() const
{
    return m_textEditShareOpenvpnCodeText;
}

void UiLogic::setTextEditShareOpenvpnCodeText(const QString &textEditShareOpenvpnCodeText)
{
    if (m_textEditShareOpenvpnCodeText != textEditShareOpenvpnCodeText) {
        m_textEditShareOpenvpnCodeText = textEditShareOpenvpnCodeText;
        emit textEditShareOpenvpnCodeTextChanged();
    }
}

bool UiLogic::getPushButtonShareOpenvpnCopyEnabled() const
{
    return m_pushButtonShareOpenvpnCopyEnabled;
}

void UiLogic::setPushButtonShareOpenvpnCopyEnabled(bool pushButtonShareOpenvpnCopyEnabled)
{
    if (m_pushButtonShareOpenvpnCopyEnabled != pushButtonShareOpenvpnCopyEnabled) {
        m_pushButtonShareOpenvpnCopyEnabled = pushButtonShareOpenvpnCopyEnabled;
        emit pushButtonShareOpenvpnCopyEnabledChanged();
    }
}

bool UiLogic::getPushButtonShareOpenvpnSaveEnabled() const
{
    return m_pushButtonShareOpenvpnSaveEnabled;
}

void UiLogic::setPushButtonShareOpenvpnSaveEnabled(bool pushButtonShareOpenvpnSaveEnabled)
{
    if (m_pushButtonShareOpenvpnSaveEnabled != pushButtonShareOpenvpnSaveEnabled) {
        m_pushButtonShareOpenvpnSaveEnabled = pushButtonShareOpenvpnSaveEnabled;
        emit pushButtonShareOpenvpnSaveEnabledChanged();
    }
}

int UiLogic::getToolBoxShareConnectionCurrentIndex() const
{
    return m_toolBoxShareConnectionCurrentIndex;
}

void UiLogic::setToolBoxShareConnectionCurrentIndex(int toolBoxShareConnectionCurrentIndex)
{
    if (m_toolBoxShareConnectionCurrentIndex != toolBoxShareConnectionCurrentIndex) {
        m_toolBoxShareConnectionCurrentIndex = toolBoxShareConnectionCurrentIndex;
        emit toolBoxShareConnectionCurrentIndexChanged();
    }
}

bool UiLogic::getPushButtonShareSsCopyEnabled() const
{
    return m_pushButtonShareSsCopyEnabled;
}

void UiLogic::setPushButtonShareSsCopyEnabled(bool pushButtonShareSsCopyEnabled)
{
    if (m_pushButtonShareSsCopyEnabled != pushButtonShareSsCopyEnabled) {
        m_pushButtonShareSsCopyEnabled = pushButtonShareSsCopyEnabled;
        emit pushButtonShareSsCopyEnabledChanged();
    }
}

QString UiLogic::getLineEditShareSsStringText() const
{
    return m_lineEditShareSsStringText;
}

void UiLogic::setLineEditShareSsStringText(const QString &lineEditShareSsStringText)
{
    if (m_lineEditShareSsStringText != lineEditShareSsStringText) {
        m_lineEditShareSsStringText = lineEditShareSsStringText;
        emit lineEditShareSsStringTextChanged();
    }
}

QString UiLogic::getLabelShareSsQrCodeText() const
{
    return m_labelShareSsQrCodeText;
}

void UiLogic::setLabelShareSsQrCodeText(const QString &labelShareSsQrCodeText)
{
    if (m_labelShareSsQrCodeText != labelShareSsQrCodeText) {
        m_labelShareSsQrCodeText = labelShareSsQrCodeText;
        emit labelShareSsQrCodeTextChanged();
    }
}

QString UiLogic::getLabelShareSsServerText() const
{
    return m_labelShareSsServerText;
}

void UiLogic::setLabelShareSsServerText(const QString &labelShareSsServerText)
{
    if (m_labelShareSsServerText != labelShareSsServerText) {
        m_labelShareSsServerText = labelShareSsServerText;
        emit labelShareSsServerTextChanged();
    }
}

QString UiLogic::getLabelShareSsPortText() const
{
    return m_labelShareSsPortText;
}

void UiLogic::setLabelShareSsPortText(const QString &labelShareSsPortText)
{
    if (m_labelShareSsPortText != labelShareSsPortText) {
        m_labelShareSsPortText = labelShareSsPortText;
        emit labelShareSsPortTextChanged();
    }
}

QString UiLogic::getLabelShareSsMethodText() const
{
    return m_labelShareSsMethodText;
}

void UiLogic::setLabelShareSsMethodText(const QString &labelShareSsMethodText)
{
    if (m_labelShareSsMethodText != labelShareSsMethodText) {
        m_labelShareSsMethodText = labelShareSsMethodText;
        emit labelShareSsMethodTextChanged();
    }
}

QString UiLogic::getLabelShareSsPasswordText() const
{
    return m_labelShareSsPasswordText;
}

void UiLogic::setLabelShareSsPasswordText(const QString &labelShareSsPasswordText)
{
    if (m_labelShareSsPasswordText != labelShareSsPasswordText) {
        m_labelShareSsPasswordText = labelShareSsPasswordText;
        emit labelShareSsPasswordTextChanged();
    }
}

QString UiLogic::getPlainTextEditShareCloakText() const
{
    return m_plainTextEditShareCloakText;
}

void UiLogic::setPlainTextEditShareCloakText(const QString &plainTextEditShareCloakText)
{
    if (m_plainTextEditShareCloakText != plainTextEditShareCloakText) {
        m_plainTextEditShareCloakText = plainTextEditShareCloakText;
        emit plainTextEditShareCloakTextChanged();
    }
}

bool UiLogic::getPushButtonShareCloakCopyEnabled() const
{
    return m_pushButtonShareCloakCopyEnabled;
}

void UiLogic::setPushButtonShareCloakCopyEnabled(bool pushButtonShareCloakCopyEnabled)
{
    if (m_pushButtonShareCloakCopyEnabled != pushButtonShareCloakCopyEnabled) {
        m_pushButtonShareCloakCopyEnabled = pushButtonShareCloakCopyEnabled;
        emit pushButtonShareCloakCopyEnabledChanged();
    }
}

QString UiLogic::getTextEditShareFullCodeText() const
{
    return m_textEditShareFullCodeText;
}

void UiLogic::setTextEditShareFullCodeText(const QString &textEditShareFullCodeText)
{
    if (m_textEditShareFullCodeText != textEditShareFullCodeText) {
        m_textEditShareFullCodeText = textEditShareFullCodeText;
        emit textEditShareFullCodeTextChanged();
    }
}

QString UiLogic::getTextEditShareAmneziaCodeText() const
{
    return m_textEditShareAmneziaCodeText;
}

void UiLogic::setTextEditShareAmneziaCodeText(const QString &textEditShareAmneziaCodeText)
{
    if (m_textEditShareAmneziaCodeText != textEditShareAmneziaCodeText) {
        m_textEditShareAmneziaCodeText = textEditShareAmneziaCodeText;
        emit textEditShareAmneziaCodeTextChanged();
    }
}

QString UiLogic::getPushButtonShareFullCopyText() const
{
    return m_pushButtonShareFullCopyText;
}

void UiLogic::setPushButtonShareFullCopyText(const QString &pushButtonShareFullCopyText)
{
    if (m_pushButtonShareFullCopyText != pushButtonShareFullCopyText) {
        m_pushButtonShareFullCopyText = pushButtonShareFullCopyText;
        emit pushButtonShareFullCopyTextChanged();
    }
}
QString UiLogic::getPushButtonShareAmneziaCopyText() const
{
    return m_pushButtonShareAmneziaCopyText;
}

void UiLogic::setPushButtonShareAmneziaCopyText(const QString &pushButtonShareAmneziaCopyText)
{
    if (m_pushButtonShareAmneziaCopyText != pushButtonShareAmneziaCopyText) {
        m_pushButtonShareAmneziaCopyText = pushButtonShareAmneziaCopyText;
        emit pushButtonShareAmneziaCopyTextChanged();
    }
}

QString UiLogic::getPushButtonShareOpenvpnCopyText() const
{
    return m_pushButtonShareOpenvpnCopyText;
}

void UiLogic::setPushButtonShareOpenvpnCopyText(const QString &pushButtonShareOpenvpnCopyText)
{
    if (m_pushButtonShareOpenvpnCopyText != pushButtonShareOpenvpnCopyText) {
        m_pushButtonShareOpenvpnCopyText = pushButtonShareOpenvpnCopyText;
        emit pushButtonShareOpenvpnCopyTextChanged();
    }
}

QString UiLogic::getPushButtonShareSsCopyText() const
{
    return m_pushButtonShareSsCopyText;
}

void UiLogic::setPushButtonShareSsCopyText(const QString &pushButtonShareSsCopyText)
{
    if (m_pushButtonShareSsCopyText != pushButtonShareSsCopyText) {
        m_pushButtonShareSsCopyText = pushButtonShareSsCopyText;
        emit pushButtonShareSsCopyTextChanged();
    }
}

QString UiLogic::getPushButtonShareCloakCopyText() const
{
    return m_pushButtonShareCloakCopyText;
}

void UiLogic::setPushButtonShareCloakCopyText(const QString &pushButtonShareCloakCopyText)
{
    if (m_pushButtonShareCloakCopyText != pushButtonShareCloakCopyText) {
        m_pushButtonShareCloakCopyText = pushButtonShareCloakCopyText;
        emit pushButtonShareCloakCopyTextChanged();
    }
}

bool UiLogic::getPushButtonShareAmneziaGenerateEnabled() const
{
    return m_pushButtonShareAmneziaGenerateEnabled;
}

void UiLogic::setPushButtonShareAmneziaGenerateEnabled(bool pushButtonShareAmneziaGenerateEnabled)
{
    if (m_pushButtonShareAmneziaGenerateEnabled != pushButtonShareAmneziaGenerateEnabled) {
        m_pushButtonShareAmneziaGenerateEnabled = pushButtonShareAmneziaGenerateEnabled;
        emit pushButtonShareAmneziaGenerateEnabledChanged();
    }
}

bool UiLogic::getPushButtonShareAmneziaCopyEnabled() const
{
    return m_pushButtonShareAmneziaCopyEnabled;
}

void UiLogic::setPushButtonShareAmneziaCopyEnabled(bool pushButtonShareAmneziaCopyEnabled)
{
    if (m_pushButtonShareAmneziaCopyEnabled != pushButtonShareAmneziaCopyEnabled) {
        m_pushButtonShareAmneziaCopyEnabled = pushButtonShareAmneziaCopyEnabled;
        emit pushButtonShareAmneziaCopyEnabledChanged();
    }
}

QString UiLogic::getPushButtonShareAmneziaGenerateText() const
{
    return m_pushButtonShareAmneziaGenerateText;
}

void UiLogic::setPushButtonShareAmneziaGenerateText(const QString &pushButtonShareAmneziaGenerateText)
{
    if (m_pushButtonShareAmneziaGenerateText != pushButtonShareAmneziaGenerateText) {
        m_pushButtonShareAmneziaGenerateText = pushButtonShareAmneziaGenerateText;
        emit pushButtonShareAmneziaGenerateTextChanged();
    }
}

bool UiLogic::getPushButtonShareOpenvpnGenerateEnabled() const
{
    return m_pushButtonShareOpenvpnGenerateEnabled;
}

void UiLogic::setPushButtonShareOpenvpnGenerateEnabled(bool pushButtonShareOpenvpnGenerateEnabled)
{
    if (m_pushButtonShareOpenvpnGenerateEnabled != pushButtonShareOpenvpnGenerateEnabled) {
        m_pushButtonShareOpenvpnGenerateEnabled = pushButtonShareOpenvpnGenerateEnabled;
        emit pushButtonShareOpenvpnGenerateEnabledChanged();
    }
}

QString UiLogic::getPushButtonShareOpenvpnGenerateText() const
{
    return m_pushButtonShareOpenvpnGenerateText;
}

void UiLogic::setPushButtonShareOpenvpnGenerateText(const QString &pushButtonShareOpenvpnGenerateText)
{
    if (m_pushButtonShareOpenvpnGenerateText != pushButtonShareOpenvpnGenerateText) {
        m_pushButtonShareOpenvpnGenerateText = pushButtonShareOpenvpnGenerateText;
        emit pushButtonShareOpenvpnGenerateTextChanged();
    }
}

bool UiLogic::getPageNewServerConfiguringEnabled() const
{
    return m_pageNewServerConfiguringEnabled;
}

void UiLogic::setPageNewServerConfiguringEnabled(bool pageNewServerConfiguringEnabled)
{
    if (m_pageNewServerConfiguringEnabled != pageNewServerConfiguringEnabled) {
        m_pageNewServerConfiguringEnabled = pageNewServerConfiguringEnabled;
        emit pageNewServerConfiguringEnabledChanged();
    }
}

bool UiLogic::getLabelNewServerConfiguringWaitInfoVisible() const
{
    return m_labelNewServerConfiguringWaitInfoVisible;
}

void UiLogic::setLabelNewServerConfiguringWaitInfoVisible(bool labelNewServerConfiguringWaitInfoVisible)
{
    if (m_labelNewServerConfiguringWaitInfoVisible != labelNewServerConfiguringWaitInfoVisible) {
        m_labelNewServerConfiguringWaitInfoVisible = labelNewServerConfiguringWaitInfoVisible;
        emit labelNewServerConfiguringWaitInfoVisibleChanged();
    }
}

QString UiLogic::getLabelNewServerConfiguringWaitInfoText() const
{
    return m_labelNewServerConfiguringWaitInfoText;
}

void UiLogic::setLabelNewServerConfiguringWaitInfoText(const QString &labelNewServerConfiguringWaitInfoText)
{
    if (m_labelNewServerConfiguringWaitInfoText != labelNewServerConfiguringWaitInfoText) {
        m_labelNewServerConfiguringWaitInfoText = labelNewServerConfiguringWaitInfoText;
        emit labelNewServerConfiguringWaitInfoTextChanged();
    }
}

bool UiLogic::getProgressBarNewServerConfiguringVisible() const
{
    return m_progressBarNewServerConfiguringVisible;
}

void UiLogic::setProgressBarNewServerConfiguringVisible(bool progressBarNewServerConfiguringVisible)
{
    if (m_progressBarNewServerConfiguringVisible != progressBarNewServerConfiguringVisible) {
        m_progressBarNewServerConfiguringVisible = progressBarNewServerConfiguringVisible;
        emit progressBarNewServerConfiguringVisibleChanged();
    }
}

int UiLogic::getProgressBarNewServerConfiguringMaximium() const
{
    return m_progressBarNewServerConfiguringMaximium;
}

void UiLogic::setProgressBarNewServerConfiguringMaximium(int progressBarNewServerConfiguringMaximium)
{
    if (m_progressBarNewServerConfiguringMaximium != progressBarNewServerConfiguringMaximium) {
        m_progressBarNewServerConfiguringMaximium = progressBarNewServerConfiguringMaximium;
        emit progressBarNewServerConfiguringMaximiumChanged();
    }
}

bool UiLogic::getProgressBarNewServerConfiguringTextVisible() const
{
    return m_progressBarNewServerConfiguringTextVisible;
}

void UiLogic::setProgressBarNewServerConfiguringTextVisible(bool progressBarNewServerConfiguringTextVisible)
{
    if (m_progressBarNewServerConfiguringTextVisible != progressBarNewServerConfiguringTextVisible) {
        m_progressBarNewServerConfiguringTextVisible = progressBarNewServerConfiguringTextVisible;
        emit progressBarNewServerConfiguringTextVisibleChanged();
    }
}

QString UiLogic::getProgressBarNewServerConfiguringText() const
{
    return m_progressBarNewServerConfiguringText;
}

void UiLogic::setProgressBarNewServerConfiguringText(const QString &progressBarNewServerConfiguringText)
{
    if (m_progressBarNewServerConfiguringText != progressBarNewServerConfiguringText) {
        m_progressBarNewServerConfiguringText = progressBarNewServerConfiguringText;
        emit progressBarNewServerConfiguringTextChanged();
    }
}

bool UiLogic::getPageServerProtocolsEnabled() const
{
    return m_pageServerProtocolsEnabled;
}

void UiLogic::setPageServerProtocolsEnabled(bool pageServerProtocolsEnabled)
{
    if (m_pageServerProtocolsEnabled != pageServerProtocolsEnabled) {
        m_pageServerProtocolsEnabled = pageServerProtocolsEnabled;
        emit pageServerProtocolsEnabledChanged();
    }
}

int UiLogic::getProgressBarProtocolsContainerReinstallValue() const
{
    return m_progressBarProtocolsContainerReinstallValue;
}

void UiLogic::setProgressBarProtocolsContainerReinstallValue(int progressBarProtocolsContainerReinstallValue)
{
    if (m_progressBarProtocolsContainerReinstallValue != progressBarProtocolsContainerReinstallValue) {
        m_progressBarProtocolsContainerReinstallValue = progressBarProtocolsContainerReinstallValue;
        emit progressBarProtocolsContainerReinstallValueChanged();
    }
}

int UiLogic::getProgressBarProtocolsContainerReinstallMaximium() const
{
    return m_progressBarProtocolsContainerReinstallMaximium;
}

void UiLogic::setProgressBarProtocolsContainerReinstallMaximium(int progressBarProtocolsContainerReinstallMaximium)
{
    if (m_progressBarProtocolsContainerReinstallMaximium != progressBarProtocolsContainerReinstallMaximium) {
        m_progressBarProtocolsContainerReinstallMaximium = progressBarProtocolsContainerReinstallMaximium;
        emit progressBarProtocolsContainerReinstallMaximiumChanged();
    }
}

bool UiLogic::getComboBoxProtoOpenvpnCipherEnabled() const
{
    return m_comboBoxProtoOpenvpnCipherEnabled;
}

void UiLogic::setComboBoxProtoOpenvpnCipherEnabled(bool comboBoxProtoOpenvpnCipherEnabled)
{
    if (m_comboBoxProtoOpenvpnCipherEnabled != comboBoxProtoOpenvpnCipherEnabled) {
        m_comboBoxProtoOpenvpnCipherEnabled = comboBoxProtoOpenvpnCipherEnabled;
        emit comboBoxProtoOpenvpnCipherEnabledChanged();
    }
}

bool UiLogic::getComboBoxProtoOpenvpnHashEnabled() const
{
    return m_comboBoxProtoOpenvpnHashEnabled;
}

void UiLogic::setComboBoxProtoOpenvpnHashEnabled(bool comboBoxProtoOpenvpnHashEnabled)
{
    if (m_comboBoxProtoOpenvpnHashEnabled != comboBoxProtoOpenvpnHashEnabled) {
        m_comboBoxProtoOpenvpnHashEnabled = comboBoxProtoOpenvpnHashEnabled;
        emit comboBoxProtoOpenvpnHashEnabledChanged();
    }
}
bool UiLogic::getPageProtoOpenvpnEnabled() const
{
    return m_pageProtoOpenvpnEnabled;
}

void UiLogic::setPageProtoOpenvpnEnabled(bool pageProtoOpenvpnEnabled)
{
    if (m_pageProtoOpenvpnEnabled != pageProtoOpenvpnEnabled) {
        m_pageProtoOpenvpnEnabled = pageProtoOpenvpnEnabled;
        emit pageProtoOpenvpnEnabledChanged();
    }
}

bool UiLogic::getLabelProtoOpenvpnInfoVisible() const
{
    return m_labelProtoOpenvpnInfoVisible;
}

void UiLogic::setLabelProtoOpenvpnInfoVisible(bool labelProtoOpenvpnInfoVisible)
{
    if (m_labelProtoOpenvpnInfoVisible != labelProtoOpenvpnInfoVisible) {
        m_labelProtoOpenvpnInfoVisible = labelProtoOpenvpnInfoVisible;
        emit labelProtoOpenvpnInfoVisibleChanged();
    }
}

QString UiLogic::getLabelProtoOpenvpnInfoText() const
{
    return m_labelProtoOpenvpnInfoText;
}

void UiLogic::setLabelProtoOpenvpnInfoText(const QString &labelProtoOpenvpnInfoText)
{
    if (m_labelProtoOpenvpnInfoText != labelProtoOpenvpnInfoText) {
        m_labelProtoOpenvpnInfoText = labelProtoOpenvpnInfoText;
        emit labelProtoOpenvpnInfoTextChanged();
    }
}

int UiLogic::getProgressBarProtoOpenvpnResetValue() const
{
    return m_progressBarProtoOpenvpnResetValue;
}

void UiLogic::setProgressBarProtoOpenvpnResetValue(int progressBarProtoOpenvpnResetValue)
{
    if (m_progressBarProtoOpenvpnResetValue != progressBarProtoOpenvpnResetValue) {
        m_progressBarProtoOpenvpnResetValue = progressBarProtoOpenvpnResetValue;
        emit progressBarProtoOpenvpnResetValueChanged();
    }
}

int UiLogic::getProgressBarProtoOpenvpnResetMaximium() const
{
    return m_progressBarProtoOpenvpnResetMaximium;
}

void UiLogic::setProgressBarProtoOpenvpnResetMaximium(int progressBarProtoOpenvpnResetMaximium)
{
    if (m_progressBarProtoOpenvpnResetMaximium != progressBarProtoOpenvpnResetMaximium) {
        m_progressBarProtoOpenvpnResetMaximium = progressBarProtoOpenvpnResetMaximium;
        emit progressBarProtoOpenvpnResetMaximiumChanged();
    }
}
bool UiLogic::getPageProtoShadowsocksEnabled() const
{
    return m_pageProtoShadowsocksEnabled;
}

void UiLogic::setPageProtoShadowsocksEnabled(bool pageProtoShadowsocksEnabled)
{
    if (m_pageProtoShadowsocksEnabled != pageProtoShadowsocksEnabled) {
        m_pageProtoShadowsocksEnabled = pageProtoShadowsocksEnabled;
        emit pageProtoShadowsocksEnabledChanged();
    }
}

bool UiLogic::getLabelProtoShadowsocksInfoVisible() const
{
    return m_labelProtoShadowsocksInfoVisible;
}

void UiLogic::setLabelProtoShadowsocksInfoVisible(bool labelProtoShadowsocksInfoVisible)
{
    if (m_labelProtoShadowsocksInfoVisible != labelProtoShadowsocksInfoVisible) {
        m_labelProtoShadowsocksInfoVisible = labelProtoShadowsocksInfoVisible;
        emit labelProtoShadowsocksInfoVisibleChanged();
    }
}

QString UiLogic::getLabelProtoShadowsocksInfoText() const
{
    return m_labelProtoShadowsocksInfoText;
}

void UiLogic::setLabelProtoShadowsocksInfoText(const QString &labelProtoShadowsocksInfoText)
{
    if (m_labelProtoShadowsocksInfoText != labelProtoShadowsocksInfoText) {
        m_labelProtoShadowsocksInfoText = labelProtoShadowsocksInfoText;
        emit labelProtoShadowsocksInfoTextChanged();
    }
}

int UiLogic::getProgressBarProtoShadowsocksResetValue() const
{
    return m_progressBarProtoShadowsocksResetValue;
}

void UiLogic::setProgressBarProtoShadowsocksResetValue(int progressBarProtoShadowsocksResetValue)
{
    if (m_progressBarProtoShadowsocksResetValue != progressBarProtoShadowsocksResetValue) {
        m_progressBarProtoShadowsocksResetValue = progressBarProtoShadowsocksResetValue;
        emit progressBarProtoShadowsocksResetValueChanged();
    }
}

int UiLogic::getProgressBarProtoShadowsocksResetMaximium() const
{
    return m_progressBarProtoShadowsocksResetMaximium;
}

void UiLogic::setProgressBarProtoShadowsocksResetMaximium(int progressBarProtoShadowsocksResetMaximium)
{
    if (m_progressBarProtoShadowsocksResetMaximium != progressBarProtoShadowsocksResetMaximium) {
        m_progressBarProtoShadowsocksResetMaximium = progressBarProtoShadowsocksResetMaximium;
        emit progressBarProtoShadowsocksResetMaximiumChanged();
    }
}
bool UiLogic::getPageProtoCloakEnabled() const
{
    return m_pageProtoCloakEnabled;
}

void UiLogic::setPageProtoCloakEnabled(bool pageProtoCloakEnabled)
{
    if (m_pageProtoCloakEnabled != pageProtoCloakEnabled) {
        m_pageProtoCloakEnabled = pageProtoCloakEnabled;
        emit pageProtoCloakEnabledChanged();
    }
}

bool UiLogic::getLabelProtoCloakInfoVisible() const
{
    return m_labelProtoCloakInfoVisible;
}

void UiLogic::setLabelProtoCloakInfoVisible(bool labelProtoCloakInfoVisible)
{
    if (m_labelProtoCloakInfoVisible != labelProtoCloakInfoVisible) {
        m_labelProtoCloakInfoVisible = labelProtoCloakInfoVisible;
        emit labelProtoCloakInfoVisibleChanged();
    }
}

QString UiLogic::getLabelProtoCloakInfoText() const
{
    return m_labelProtoCloakInfoText;
}

void UiLogic::setLabelProtoCloakInfoText(const QString &labelProtoCloakInfoText)
{
    if (m_labelProtoCloakInfoText != labelProtoCloakInfoText) {
        m_labelProtoCloakInfoText = labelProtoCloakInfoText;
        emit labelProtoCloakInfoTextChanged();
    }
}

int UiLogic::getProgressBarProtoCloakResetValue() const
{
    return m_progressBarProtoCloakResetValue;
}

void UiLogic::setProgressBarProtoCloakResetValue(int progressBarProtoCloakResetValue)
{
    if (m_progressBarProtoCloakResetValue != progressBarProtoCloakResetValue) {
        m_progressBarProtoCloakResetValue = progressBarProtoCloakResetValue;
        emit progressBarProtoCloakResetValueChanged();
    }
}

int UiLogic::getProgressBarProtoCloakResetMaximium() const
{
    return m_progressBarProtoCloakResetMaximium;
}

void UiLogic::setProgressBarProtoCloakResetMaximium(int progressBarProtoCloakResetMaximium)
{
    if (m_progressBarProtoCloakResetMaximium != progressBarProtoCloakResetMaximium) {
        m_progressBarProtoCloakResetMaximium = progressBarProtoCloakResetMaximium;
        emit progressBarProtoCloakResetMaximiumChanged();
    }
}

QString UiLogic::getPushButtonServerSettingsClearClientCacheText() const
{
    return m_pushButtonServerSettingsClearClientCacheText;
}

void UiLogic::setPushButtonServerSettingsClearClientCacheText(const QString &pushButtonServerSettingsClearClientCacheText)
{
    if (m_pushButtonServerSettingsClearClientCacheText != pushButtonServerSettingsClearClientCacheText) {
        m_pushButtonServerSettingsClearClientCacheText = pushButtonServerSettingsClearClientCacheText;
        emit pushButtonServerSettingsClearClientCacheTextChanged();
    }
}

QObject* UiLogic::getServerListModel() const
{
    return m_serverListModel;
}

UiLogic::~UiLogic()
{
    hide();
    m_vpnConnection->disconnectFromVpn();
    for (int i = 0; i < 50; i++) {
        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
        QThread::msleep(100);
        if (m_vpnConnection->isDisconnected()) {
            break;
        }
    }

    delete m_vpnConnection;

    qDebug() << "Application closed";
}

void UiLogic::showOnStartup()
{
    if (! m_settings.isStartMinimized()) {
        show();
    } else {
#if defined Q_OS_MACX
        setDockIconVisible(false);
#endif
    }
}


//void UiLogic::keyPressEvent(QKeyEvent *event)
//{
//    switch (event->key()) {
//    case Qt::Key_L:
//        if (!Debug::openLogsFolder()) {
//            QMessageBox::warning(this, APPLICATION_NAME, tr("Cannot open logs folder!"));
//        }
//        break;
//#ifdef QT_DEBUG
//    case Qt::Key_Q:
//        qApp->quit();
//        break;
//        //    case Qt::Key_0:
//        //        *((char*)-1) = 'x';
//        //        break;
//    case Qt::Key_H:
//        selectedServerIndex = m_settings.defaultServerIndex();
//        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);

//        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);
//        goToPage(Page::ShareConnection);
//        break;
//#endif
//    case Qt::Key_C:
//        qDebug().noquote() << "Def server" << m_settings.defaultServerIndex() << m_settings.defaultContainerName(m_settings.defaultServerIndex());
//        //qDebug().noquote() << QJsonDocument(m_settings.containerConfig(m_settings.defaultServerIndex(), m_settings.defaultContainer(m_settings.defaultServerIndex()))).toJson();
//        qDebug().noquote() << QJsonDocument(m_settings.defaultServer()).toJson();
//        break;
//    case Qt::Key_A:
//        goToPage(Page::Start);
//        break;
//    case Qt::Key_S:
//        selectedServerIndex = m_settings.defaultServerIndex();
//        goToPage(Page::ServerSettings);
//        break;
//    case Qt::Key_P:
//        selectedServerIndex = m_settings.defaultServerIndex();
//        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
//        goToPage(Page::ServerVpnProtocols);
//        break;
//    case Qt::Key_T:
//        SshConfigurator::openSshTerminal(m_settings.serverCredentials(m_settings.defaultServerIndex()));
//        break;
//    case Qt::Key_Escape:
//        if (currentPage() == Page::Vpn) break;
//        if (currentPage() == Page::ServerConfiguring) break;
//        if (currentPage() == Page::Start && pagesStack.size() < 2) break;
//        if (currentPage() == Page::Sites &&
//                ui->tableView_sites->selectionModel()->selection().indexes().size() > 0) {
//            ui->tableView_sites->clearSelection();
//            break;
//        }

//        if (! ui->stackedWidget_main->isAnimationRunning() && ui->stackedWidget_main->currentWidget()->isEnabled()) {
//            closePage();
//        }
//    default:
//        ;
//    }
//}

void UiLogic::onCloseWindow()
{
    if (m_settings.serversCount() == 0) qApp->quit();
    else {
        hide();
    }
}

void UiLogic::onServerListPushbuttonDefaultClicked(int index)
{
    m_settings.setDefaultServer(index);
    updateServersListPage();
}

void UiLogic::onServerListPushbuttonSettingsClicked(int index)
{
    selectedServerIndex = index;
    goToPage(Page::ServerSettings);
}

void UiLogic::onPushButtonServerSettingsShareFullClicked()
{
    updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), DockerContainer::None);
    goToPage(Page::ShareConnection);
}

//void UiLogic::showEvent(QShowEvent *event)
//{
//#if defined Q_OS_MACX
//    if (!event->spontaneous()) {
//        setDockIconVisible(true);
//    }
//    if (needToHideCustomTitlebar) {
//        ui->widget_tittlebar->hide();
//        resize(width(), 640);
//        ui->stackedWidget_main->move(0,0);
//    }
//#endif
//}

//void UiLogic::hideEvent(QHideEvent *event)
//{
//#if defined Q_OS_MACX
//    if (!event->spontaneous()) {
//        setDockIconVisible(false);
//    }
//#endif
//}

void UiLogic::onPushButtonNewServerConnect()
{
    if (getPushButtonNewServerConnectKeyChecked()){
        if (getLineEditNewServerIpText().isEmpty() ||
                getLineEditNewServerLoginText().isEmpty() ||
                getTextEditNewServerSshKeyText().isEmpty() ) {
            setLabelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    else {
        if (getLineEditNewServerIpText().isEmpty() ||
                getLineEditNewServerLoginText().isEmpty() ||
                getLineEditNewServerPasswordText().isEmpty() ) {
            setLabelNewServerWaitInfoText(tr("Please fill in all fields"));
            return;
        }
    }
    qDebug() << "UiLogic::onPushButtonNewServerConnect checking new server";

    ServerCredentials serverCredentials;
    serverCredentials.hostName = getLineEditNewServerIpText();
    if (serverCredentials.hostName.contains(":")) {
        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
    }
    serverCredentials.userName = getLineEditNewServerLoginText();
    if (getPushButtonNewServerConnectKeyChecked()){
        QString key = getTextEditNewServerSshKeyText();
        if (key.startsWith("ssh-rsa")) {
            emit showPublicKeyWarning();
            return;
        }

        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
            key = SshConfigurator::convertOpenSShKey(key);
        }

        serverCredentials.password = key;
    }
    else {
        serverCredentials.password = getLineEditNewServerPasswordText();
    }

    setPushButtonNewServerConnectEnabled(false);
    setPushButtonNewServerConnectText(tr("Connecting..."));

    ErrorCode e = ErrorCode::NoError;
#ifdef Q_DEBUG
    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
#else
    QString output;
#endif

    bool ok = true;
    if (e) {
        setLabelNewServerWaitInfoVisible(true);
        setLabelNewServerWaitInfoText(errorString(e));
        ok = false;
    }
    else {
        if (output.contains("Please login as the user")) {
            output.replace("\n", "");
            setLabelNewServerWaitInfoVisible(true);
            setLabelNewServerWaitInfoText(output);
            ok = false;
        }
    }

    setPushButtonNewServerConnectEnabled(true);
    setPushButtonNewServerConnectText(tr("Connect"));

    installCredentials = serverCredentials;
    if (ok) goToPage(Page::NewServer);
}

QMap<DockerContainer, QJsonObject> UiLogic::getInstallConfigsFromProtocolsPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::port, getLineEditNewServerCloakPortText() },
                { config_key::site, getLineEditNewServerCloakSiteText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) },
        { config_key::shadowsocks, QJsonObject {
                { config_key::port, getLineEditNewServerSsPortText() },
                { config_key::cipher, getComboBoxNewServerSsCipherText() }}
        }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) },
        { config_key::openvpn, QJsonObject {
                { config_key::port, getlineEditNewServerOpenvpnPortText() },
                { config_key::transport_proto, getComboBoxNewServerOpenvpnProtoText() }}
        }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (getCheckBoxNewServerCloakChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (getCheckBoxNewServerSsChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (getCheckBoxNewServerOpenvpnChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

QMap<DockerContainer, QJsonObject> UiLogic::getInstallConfigsFromWizardPage() const
{
    QJsonObject cloakConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverCloak) },
        { config_key::cloak, QJsonObject {
                { config_key::site, getLineEditSetupWizardHighWebsiteMaskingText() }}
        }
    };
    QJsonObject ssConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpnOverShadowSocks) }
    };
    QJsonObject openVpnConfig {
        { config_key::container, amnezia::containerToString(DockerContainer::OpenVpn) }
    };

    QMap<DockerContainer, QJsonObject> containers;

    if (getRadioButtonSetupWizardHighChecked()) {
        containers.insert(DockerContainer::OpenVpnOverCloak, cloakConfig);
    }

    if (getRadioButtonSetupWizardMediumChecked()) {
        containers.insert(DockerContainer::OpenVpnOverShadowSocks, ssConfig);
    }

    if (getRadioButtonSetupWizardLowChecked()) {
        containers.insert(DockerContainer::OpenVpn, openVpnConfig);
    }

    return containers;
}

void UiLogic::installServer(const QMap<DockerContainer, QJsonObject> &containers)
{
    if (containers.isEmpty()) return;

    goToPage(Page::ServerConfiguring);
    QEventLoop loop;
    QTimer::singleShot(500, &loop, SLOT(quit()));
    loop.exec();
    qApp->processEvents();

    PageFunc page_new_server_configuring;
    page_new_server_configuring.setEnabledFunc = [this] (bool enabled) -> void {
        setPageNewServerConfiguringEnabled(enabled);
    };
    ButtonFunc no_button;
    LabelFunc label_new_server_configuring_wait_info;
    label_new_server_configuring_wait_info.setTextFunc = [this] (const QString& text) -> void {
        setLabelNewServerConfiguringWaitInfoText(text);
    };
    label_new_server_configuring_wait_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelNewServerConfiguringWaitInfoVisible(visible);
    };
    ProgressFunc progressBar_new_server_configuring;
    progressBar_new_server_configuring.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarNewServerConfiguringVisible(visible);
    };
    progressBar_new_server_configuring.setValueFunc = [this] (int value) ->void {
        setProgressBarNewServerConfiguringValue(value);
    };
    progressBar_new_server_configuring.getValueFunc = [this] (void) -> int {
        return getProgressBarNewServerConfiguringValue();
    };
    progressBar_new_server_configuring.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarNewServerConfiguringMaximium();
    };
    progressBar_new_server_configuring.setTextVisibleFunc = [this] (bool visible) ->void {
        setProgressBarNewServerConfiguringTextVisible(visible);
    };
    progressBar_new_server_configuring.setTextFunc = [this] (const QString& text) ->void {
        setProgressBarNewServerConfiguringText(text);
    };
    bool ok = installContainers(installCredentials, containers,
                                page_new_server_configuring,
                                progressBar_new_server_configuring,
                                no_button,
                                label_new_server_configuring_wait_info);

    if (ok) {
        QJsonObject server;
        server.insert(config_key::hostName, installCredentials.hostName);
        server.insert(config_key::userName, installCredentials.userName);
        server.insert(config_key::password, installCredentials.password);
        server.insert(config_key::port, installCredentials.port);
        server.insert(config_key::description, m_settings.nextAvailableServerName());

        QJsonArray containerConfigs;
        for (const QJsonObject &cfg : containers) {
            containerConfigs.append(cfg);
        }
        server.insert(config_key::containers, containerConfigs);
        server.insert(config_key::defaultContainer, containerToString(containers.firstKey()));

        m_settings.addServer(server);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        setStartPage(Page::Vpn);
        qApp->processEvents();
    }
    else {
        closePage();
    }
}

void UiLogic::onPushButtonNewServerImport()
{
    QString s = getLineEditStartExistingCodeText();
    s.replace("vpn://", "");
    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromBase64(s.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)).object();

    ServerCredentials credentials;
    credentials.hostName = o.value("h").toString();
    if (credentials.hostName.isEmpty()) credentials.hostName = o.value(config_key::hostName).toString();

    credentials.port = o.value("p").toInt();
    if (credentials.port == 0) credentials.port = o.value(config_key::port).toInt();

    credentials.userName = o.value("u").toString();
    if (credentials.userName.isEmpty()) credentials.userName = o.value(config_key::userName).toString();

    credentials.password = o.value("w").toString();
    if (credentials.password.isEmpty()) credentials.password = o.value(config_key::password).toString();

    if (credentials.isValid()) {
        o.insert(config_key::hostName, credentials.hostName);
        o.insert(config_key::port, credentials.port);
        o.insert(config_key::userName, credentials.userName);
        o.insert(config_key::password, credentials.password);

        o.remove("h");
        o.remove("p");
        o.remove("u");
        o.remove("w");
    }
    qDebug() << QString("Added server %3@%1:%2").
                arg(credentials.hostName).
                arg(credentials.port).
                arg(credentials.userName);

    //qDebug() << QString("Password") << credentials.password;

    if (credentials.isValid() || o.contains(config_key::containers)) {
        m_settings.addServer(o);
        m_settings.setDefaultServer(m_settings.serversCount() - 1);

        setStartPage(Page::Vpn);
    }
    else {
        qDebug() << "Failed to import profile";
        qDebug().noquote() << QJsonDocument(o).toJson();
        return;
    }

    if (!o.contains(config_key::containers)) {
        selectedServerIndex = m_settings.defaultServerIndex();
        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
        goToPage(Page::ServerVpnProtocols);
    }
}

bool UiLogic::installContainers(ServerCredentials credentials,
                                const QMap<DockerContainer, QJsonObject> &containers,
                                const PageFunc &page,
                                const ProgressFunc &progress,
                                const ButtonFunc &button,
                                const LabelFunc &info)
{
    if (!progress.setValueFunc) return false;

    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (button.setVisibleFunc) {
        button.setVisibleFunc(false);
    }

    if (info.setVisibleFunc) {
        info.setVisibleFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    int cnt = 0;
    for (QMap<DockerContainer, QJsonObject>::const_iterator i = containers.constBegin(); i != containers.constEnd(); i++, cnt++) {
        QTimer timer;
        connect(&timer, &QTimer::timeout, [progress](){
            progress.setValueFunc(progress.getValueFunc() + 1);
        });

        progress.setValueFunc(0);
        timer.start(1000);

        progress.setTextVisibleFunc(true);
        progress.setTextFunc(QString("Installing %1 %2 %3").arg(cnt+1).arg(tr("of")).arg(containers.size()));

        ErrorCode e = ServerController::setupContainer(credentials, i.key(), i.value());
        qDebug() << "Setup server finished with code" << e;
        ServerController::disconnectFromHost(credentials);

        if (e) {
            if (page.setEnabledFunc) {
                page.setEnabledFunc(true);
            }
            if (button.setVisibleFunc) {
                button.setVisibleFunc(true);
            }
            if (info.setVisibleFunc) {
                info.setVisibleFunc(false);
            }

            QMessageBox::warning(nullptr, APPLICATION_NAME,
                                 tr("Error occurred while configuring server.") + "\n" +
                                 errorString(e));

            return false;
        }

        // just ui progressbar tweak
        timer.stop();

        int remaining_val = progress.getMaximiumFunc() - progress.getValueFunc();

        if (remaining_val > 0) {
            QTimer timer1;
            QEventLoop loop1;

            connect(&timer1, &QTimer::timeout, [&](){
                progress.setValueFunc(progress.getValueFunc() + 1);
                if (progress.getValueFunc() >= progress.getMaximiumFunc()) {
                    loop1.quit();
                }
            });

            timer1.start(5);
            loop1.exec();
        }
    }


    if (button.setVisibleFunc) {
        button.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Amnezia server installed"));
    }

    return true;
}

ErrorCode UiLogic::doInstallAction(const std::function<ErrorCode()> &action,
                                   const PageFunc &page,
                                   const ProgressFunc &progress,
                                   const ButtonFunc &button,
                                   const LabelFunc &info)
{
    progress.setVisibleFunc(true);
    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (button.setVisibleFunc) {
        button.setVisibleFunc(false);
    }
    if (info.setVisibleFunc) {
        info.setVisibleFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress.setValueFunc(progress.getValueFunc() + 1);
    });

    progress.setValueFunc(0);
    timer.start(1000);

    ErrorCode e = action();
    qDebug() << "doInstallAction finished with code" << e;

    if (e) {
        if (page.setEnabledFunc) {
            page.setEnabledFunc(true);
        }
        if (button.setVisibleFunc) {
            button.setVisibleFunc(true);
        }
        if (info.setVisibleFunc) {
            info.setVisibleFunc(false);
        }
        QMessageBox::warning(nullptr, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));

        progress.setVisibleFunc(false);
        return e;
    }

    // just ui progressbar tweak
    timer.stop();

    int remaining_val = progress.getMaximiumFunc() - progress.getValueFunc();

    if (remaining_val > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            progress.setValueFunc(progress.getValueFunc() + 1);
            if (progress.getValueFunc() >= progress.getMaximiumFunc()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }


    progress.setVisibleFunc(false);
    if (button.setVisibleFunc) {
        button.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Operation finished"));
    }
    return ErrorCode::NoError;
}

void UiLogic::onPushButtonClearServer()
{
    setPageServerSettingsEnabled(false);
    setPushButtonServerSettingsClearText(tr("Uninstalling Amnezia software..."));

    if (m_settings.defaultServerIndex() == selectedServerIndex) {
        onDisconnect();
    }

    ErrorCode e = ServerController::removeAllContainers(m_settings.serverCredentials(selectedServerIndex));
    ServerController::disconnectFromHost(m_settings.serverCredentials(selectedServerIndex));
    if (e) {
        setDialogConnectErrorText(
                    tr("Error occurred while configuring server.") + "\n" +
                    errorString(e) + "\n" +
                    tr("See logs for details."));
        emit showConnectErrorDialog();
    }
    else {
        setLabelServerSettingsWaitInfoVisible(true);
        setLabelServerSettingsWaitInfoText(tr("Amnezia server successfully uninstalled"));
    }

    m_settings.setContainers(selectedServerIndex, {});
    m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);

    setPageServerSettingsEnabled(true);
    setPushButtonServerSettingsClearText(tr("Clear server from Amnezia software"));
}

void UiLogic::onPushButtonForgetServer()
{
    if (m_settings.defaultServerIndex() == selectedServerIndex && m_vpnConnection->isConnected()) {
        onDisconnect();
    }
    m_settings.removeServer(selectedServerIndex);

    if (m_settings.defaultServerIndex() == selectedServerIndex) {
        m_settings.setDefaultServer(0);
    }
    else if (m_settings.defaultServerIndex() > selectedServerIndex) {
        m_settings.setDefaultServer(m_settings.defaultServerIndex() - 1);
    }

    if (m_settings.serversCount() == 0) {
        m_settings.setDefaultServer(-1);
    }


    selectedServerIndex = -1;

    updateServersListPage();

    if (m_settings.serversCount() == 0) {
        setStartPage(Page::Start);
    }
    else {
        closePage();
    }
}

void UiLogic::onPushButtonServerSettingsClearClientCacheClicked()
{
    setPushButtonServerSettingsClearClientCacheText(tr("Cache cleared"));

    const auto &containers = m_settings.containers(selectedServerIndex);
    for (DockerContainer container: containers.keys()) {
        m_settings.clearLastConnectionConfig(selectedServerIndex, container);
    }

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonServerSettingsClearClientCacheText(tr("Clear client cached profile"));
    });
}

void UiLogic::onLineEditServerSettingsDescriptionEditingFinished()
{
    const QString &newText = getLineEditServerSettingsDescriptionText();
    QJsonObject server = m_settings.server(selectedServerIndex);
    server.insert(config_key::description, newText);
    m_settings.editServer(selectedServerIndex, server);
    updateServersListPage();
}

void UiLogic::onBytesChanged(quint64 receivedData, quint64 sentData)
{
    setLabelSpeedReceivedText(VpnConnection::bytesPerSecToText(receivedData));
    setLabelSpeedSentText(VpnConnection::bytesPerSecToText(sentData));
}

void UiLogic::onConnectionStateChanged(VpnProtocol::ConnectionState state)
{
    qDebug() << "UiLogic::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);

    bool pushButtonConnectEnabled = false;
    bool radioButtonsModeEnabled = false;
    setLabelStateText(VpnProtocol::textConnectionState(state));

    setTrayState(state);

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

void UiLogic::onVpnProtocolError(ErrorCode errorCode)
{
    setLabelErrorText(errorString(errorCode));
}

void UiLogic::onPushButtonConnectClicked(bool checked)
{
    if (checked) {
        onConnect();
    } else {
        onDisconnect();
    }
}

void UiLogic::setupTray()
{
    setTrayState(VpnProtocol::Disconnected);
}

void UiLogic::setTrayIcon(const QString &iconPath)
{
    setTrayIconUrl(iconPath);
}

PageEnumNS::Page UiLogic::currentPage()
{
    return static_cast<PageEnumNS::Page>(getCurrentPageValue());
}

void UiLogic::setupNewServerConnections()
{
    connect(this, &UiLogic::pushButtonNewServerConnectConfigureClicked, this, [this](){
        installServer(getInstallConfigsFromProtocolsPage());
    });
}

void UiLogic::setupProtocolsPageConnections()
{
    QJsonObject openvpnConfig;

    // all containers
    QList<DockerContainer> containers {
        DockerContainer::OpenVpn,
                DockerContainer::OpenVpnOverShadowSocks,
                DockerContainer::OpenVpnOverCloak,
                DockerContainer::WireGuard
    };
    using ButtonClickedFunc = void (UiLogic::*)(bool);
    using ButtonSetEnabledFunc = std::function<void(bool)>;

    // default buttons
    QList<ButtonClickedFunc> defaultButtonClickedSig {
        &UiLogic::pushButtonProtoOpenvpnContDefaultClicked,
                &UiLogic::pushButtonProtoSsOpenvpnContDefaultClicked,
                &UiLogic::pushButtonProtoCloakOpenvpnContDefaultClicked,
                &UiLogic::pushButtonProtoWireguardContDefaultClicked
    };

    for (int i = 0; i < containers.size(); ++i) {
        connect(this, defaultButtonClickedSig.at(i), [this, containers, i](bool){
            qDebug() << "clmm" << i;
            m_settings.setDefaultContainer(selectedServerIndex, containers.at(i));
            updateProtocolsPage();
        });
    }

    // install buttons
    QList<ButtonClickedFunc> installButtonsClickedSig {
        &UiLogic::pushButtonProtoOpenvpnContInstallClicked,
                &UiLogic::pushButtonProtoSsOpenvpnContInstallClicked,
                &UiLogic::pushButtonProtoCloakOpenvpnContInstallClicked,
                &UiLogic::pushButtonProtoWireguardContInstallClicked,
    };
    QList<ButtonSetEnabledFunc> installButtonsSetEnabledFunc {
        [this] (bool enabled) -> void {
            setPushButtonProtoOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoSsOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoCloakOpenvpnContInstallEnabled(enabled);
        },
        [this] (bool enabled) -> void {
            setPushButtonProtoWireguardContInstallEnabled(enabled);
        },
    };

    for (int i = 0; i < containers.size(); ++i) {
        ButtonClickedFunc buttonClickedFunc = installButtonsClickedSig.at(i);
        ButtonSetEnabledFunc buttonSetEnabledFunc = installButtonsSetEnabledFunc.at(i);
        DockerContainer container = containers.at(i);

        connect(this, buttonClickedFunc, [this, container, buttonSetEnabledFunc](bool checked){
            if (checked) {
                PageFunc page_server_protocols;
                page_server_protocols.setEnabledFunc = [this] (bool enabled) -> void {
                    setPageServerProtocolsEnabled(enabled);
                };
                ButtonFunc no_button;
                LabelFunc no_label;
                ProgressFunc progressBar_protocols_container_reinstall;
                progressBar_protocols_container_reinstall.setVisibleFunc = [this] (bool visible) ->void {
                    setProgressBarProtocolsContainerReinstallVisible(visible);
                };
                progressBar_protocols_container_reinstall.setValueFunc = [this] (int value) ->void {
                    setProgressBarProtocolsContainerReinstallValue(value);
                };
                progressBar_protocols_container_reinstall.getValueFunc = [this] (void) -> int {
                    return getProgressBarProtocolsContainerReinstallValue();
                };
                progressBar_protocols_container_reinstall.getMaximiumFunc = [this] (void) -> int {
                    return getProgressBarProtocolsContainerReinstallMaximium();
                };

                ErrorCode e = doInstallAction([this, container](){
                    return ServerController::setupContainer(m_settings.serverCredentials(selectedServerIndex), container);
                },
                page_server_protocols, progressBar_protocols_container_reinstall,
                no_button, no_label);

                if (!e) {
                    m_settings.setContainerConfig(selectedServerIndex, container, QJsonObject());
                    m_settings.setDefaultContainer(selectedServerIndex, container);
                }
            }
            else {
                buttonSetEnabledFunc(false);
                ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(selectedServerIndex), container);
                m_settings.removeContainerConfig(selectedServerIndex, container);
                buttonSetEnabledFunc(true);

                if (m_settings.defaultContainer(selectedServerIndex) == container) {
                    const auto &c = m_settings.containers(selectedServerIndex);
                    if (c.isEmpty()) m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);
                    else m_settings.setDefaultContainer(selectedServerIndex, c.keys().first());
                }
            }

            updateProtocolsPage();
        });
    }

    // share buttons
    QList<ButtonClickedFunc> shareButtonsClickedSig {
        &UiLogic::pushButtonProtoOpenvpnContShareClicked,
                &UiLogic::pushButtonProtoSsOpenvpnContShareClicked,
                &UiLogic::pushButtonProtoCloakOpenvpnContShareClicked,
                &UiLogic::pushButtonProtoWireguardContShareClicked,
    };

    for (int i = 0; i < containers.size(); ++i) {
        ButtonClickedFunc buttonClickedFunc = shareButtonsClickedSig.at(i);
        DockerContainer container = containers.at(i);

        connect(this, buttonClickedFunc, [this, container](bool){
            updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), container);
            goToPage(Page::ShareConnection);
        });
    }
}

void UiLogic::setTrayState(VpnProtocol::ConnectionState state)
{
    QString resourcesPath = "qrc:/images/tray/%1";

    setTrayActionDisconnectEnabled(state == VpnProtocol::Connected);
    setTrayActionConnectEnabled(state == VpnProtocol::Disconnected);

    switch (state) {
    case VpnProtocol::Disconnected:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Preparing:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Connecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Connected:
        setTrayIcon(QString(resourcesPath).arg(ConnectedTrayIconName));
        break;
    case VpnProtocol::Disconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Reconnecting:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
        break;
    case VpnProtocol::Error:
        setTrayIcon(QString(resourcesPath).arg(ErrorTrayIconName));
        break;
    case VpnProtocol::Unknown:
    default:
        setTrayIcon(QString(resourcesPath).arg(DisconnectedTrayIconName));
    }

    //#ifdef Q_OS_MAC
    //    // Get theme from current user (note, this app can be launched as root application and in this case this theme can be different from theme of real current user )
    //    bool darkTaskBar = MacOSFunctions::instance().isMenuBarUseDarkTheme();
    //    darkTaskBar = forceUseBrightIcons ? true : darkTaskBar;
    //    resourcesPath = ":/images_mac/tray_icon/%1";
    //    useIconName = useIconName.replace(".png", darkTaskBar ? "@2x.png" : " dark@2x.png");
    //#endif

}

void UiLogic::onConnect()
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

void UiLogic::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
{
    setLabelErrorText("");
    setPushButtonConnectChecked(true);
    qApp->processEvents();

    ErrorCode errorCode = m_vpnConnection->connectToVpn(
                serverIndex, credentials, container, containerConfig
                );

    if (errorCode) {
        //ui->pushButton_connect->setChecked(false);
        setDialogConnectErrorText(errorString(errorCode));
        emit showConnectErrorDialog();
        return;
    }

    setPushButtonConnectEnabled(false);
}

void UiLogic::onDisconnect()
{
    setPushButtonConnectChecked(false);
    m_vpnConnection->disconnectFromVpn();
}




void UiLogic::onPushButtonShareFullCopyClicked()
{
    QGuiApplication::clipboard()->setText(getTextEditShareFullCodeText());
    setPushButtonShareFullCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareFullCopyText(tr("Copy"));
    });
}

void UiLogic::onPushButtonShareFullSaveClicked()
{
    if (getTextEditShareFullCodeText().isEmpty()) return;

    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save AmneziaVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareFullCodeText().toUtf8());
    save.commit();
}

void UiLogic::onPushButtonShareAmneziaCopyClicked()
{
    if (getTextEditShareAmneziaCodeText().isEmpty()) return;

    QGuiApplication::clipboard()->setText(getTextEditShareAmneziaCodeText());
    setPushButtonShareAmneziaCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareAmneziaCopyText(tr("Copy"));
    });
}

void UiLogic::onPushButtonShareAmneziaSaveClicked()
{
    if (getTextEditShareAmneziaCodeText().isEmpty()) return;

    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save AmneziaVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareAmneziaCodeText().toUtf8());
    save.commit();
}

void UiLogic::onPushButtonShareOpenvpnCopyClicked()
{
    QGuiApplication::clipboard()->setText(getTextEditShareOpenvpnCodeText());
    setPushButtonShareOpenvpnCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareOpenvpnCopyText(tr("Copy"));
    });
}

void UiLogic::onPushButtonShareSsCopyClicked()
{
    QGuiApplication::clipboard()->setText(getLineEditShareSsStringText());
    setPushButtonShareSsCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareSsCopyText(tr("Copy"));
    });
}

void UiLogic::onPushButtonShareCloakCopyClicked()
{
    QGuiApplication::clipboard()->setText(getPlainTextEditShareCloakText());
    setPushButtonShareCloakCopyText(tr("Copied"));

    QTimer::singleShot(3000, this, [this]() {
        setPushButtonShareCloakCopyText(tr("Copy"));
    });
}

void UiLogic::onPushButtonShareAmneziaGenerateClicked()
{
    setPushButtonShareAmneziaGenerateEnabled(false);
    setPushButtonShareAmneziaCopyEnabled(false);
    setPushButtonShareAmneziaGenerateText(tr("Generating..."));
    qApp->processEvents();

    ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
    QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
    containerConfig.insert(config_key::container, containerToString(selectedDockerContainer));

    ErrorCode e = ErrorCode::NoError;
    for (Protocol p: amnezia::protocolsForContainer(selectedDockerContainer)) {
        QJsonObject protoConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, p);

        QString cfg = VpnConfigurator::genVpnProtocolConfig(credentials, selectedDockerContainer, containerConfig, p, &e);
        if (e) {
            cfg = "Error generating config";
            break;
        }
        protoConfig.insert(config_key::last_config, cfg);

        containerConfig.insert(protoToString(p), protoConfig);
    }

    QByteArray ba;
    if (!e) {
        QJsonObject serverConfig = m_settings.server(selectedServerIndex);
        serverConfig.remove(config_key::userName);
        serverConfig.remove(config_key::password);
        serverConfig.remove(config_key::port);
        serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
        serverConfig.insert(config_key::defaultContainer, containerToString(selectedDockerContainer));


        ba = QJsonDocument(serverConfig).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        setTextEditShareAmneziaCodeText(QString("vpn://%1").arg(QString(ba)));
    }
    else {
        setTextEditShareAmneziaCodeText(tr("Error while generating connection profile"));
    }

    setPushButtonShareAmneziaGenerateEnabled(true);
    setPushButtonShareAmneziaCopyEnabled(true);
    setPushButtonShareAmneziaGenerateText(tr("Generate config"));
}

void UiLogic::onPushButtonShareOpenvpnGenerateClicked()
{
    setPushButtonShareOpenvpnGenerateEnabled(false);
    setPushButtonShareOpenvpnCopyEnabled(false);
    setPushButtonShareOpenvpnSaveEnabled(false);
    setPushButtonShareOpenvpnGenerateText(tr("Generating..."));

    ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
    const QJsonObject &containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);

    ErrorCode e = ErrorCode::NoError;
    QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, selectedDockerContainer, containerConfig, &e);
    cfg = OpenVpnConfigurator::processConfigWithExportSettings(cfg);

    setTextEditShareOpenvpnCodeText(cfg);

    setPushButtonShareOpenvpnGenerateEnabled(true);
    setPushButtonShareOpenvpnCopyEnabled(true);
    setPushButtonShareOpenvpnSaveEnabled(true);
    setPushButtonShareOpenvpnGenerateText(tr("Generate config"));
}

void UiLogic::onPushButtonShareOpenvpnSaveClicked()
{
    QString fileName = QFileDialog::getSaveFileName(nullptr, tr("Save OpenVPN config"),
                                                    QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.ovpn");

    QSaveFile save(fileName);
    save.open(QIODevice::WriteOnly);
    save.write(getTextEditShareOpenvpnCodeText().toUtf8());
    save.commit();
}



void UiLogic::onPushButtonProtoOpenvpnContOpenvpnConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpn;
    updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
                      selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::OpenVpnSettings);
}

void UiLogic::onPushButtonProtoSsOpenvpnContOpenvpnConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
    updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
                      selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::OpenVpnSettings);
}

void UiLogic::onPushButtonProtoSsOpenvpnContSsConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
    updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
                          selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::ShadowSocksSettings);
}

void UiLogic::onPushButtonProtoCloakOpenvpnContOpenvpnConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
                      selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::OpenVpnSettings);
}

void UiLogic::onPushButtonProtoCloakOpenvpnContSsConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
                          selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::ShadowSocksSettings);
}

void UiLogic::onPushButtonProtoCloakOpenvpnContCloakConfigClicked()
{
    selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
    updateCloakPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak),
                    selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
    goToPage(Page::CloakSettings);
}

void UiLogic::onCheckBoxProtoOpenvpnAutoEncryptionClicked()
{
    setComboBoxProtoOpenvpnCipherEnabled(!getCheckBoxProtoOpenvpnAutoEncryptionChecked());
    setComboBoxProtoOpenvpnHashEnabled(!getCheckBoxProtoOpenvpnAutoEncryptionChecked());
}

void UiLogic::onPushButtonProtoOpenvpnSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn);
    protocolConfig = getOpenVpnConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::openvpn, protocolConfig);

    PageFunc page_proto_openvpn;
    page_proto_openvpn.setEnabledFunc = [this] (bool enabled) -> void {
        setPageProtoOpenvpnEnabled(enabled);
    };
    ButtonFunc pushButton_proto_openvpn_save;
    pushButton_proto_openvpn_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoOpenvpnSaveVisible(visible);
    };
    LabelFunc label_proto_openvpn_info;
    label_proto_openvpn_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoOpenvpnInfoVisible(visible);
    };
    label_proto_openvpn_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoOpenvpnInfoText(text);
    };
    ProgressFunc progressBar_proto_openvpn_reset;
    progressBar_proto_openvpn_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoOpenvpnResetVisible(visible);
    };
    progressBar_proto_openvpn_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoOpenvpnResetValue(value);
    };
    progressBar_proto_openvpn_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoOpenvpnResetValue();
    };
    progressBar_proto_openvpn_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoOpenvpnResetMaximium();
    };

    ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_openvpn, progressBar_proto_openvpn_reset,
    pushButton_proto_openvpn_save, label_proto_openvpn_info);

    if (!e) {
        m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
}

void UiLogic::onPushButtonProtoShadowsocksSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks);
    protocolConfig = getShadowSocksConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::shadowsocks, protocolConfig);
    PageFunc page_proto_shadowsocks;
    page_proto_shadowsocks.setEnabledFunc = [this] (bool enabled) -> void {
        setPageProtoShadowsocksEnabled(enabled);
    };
    ButtonFunc pushButton_proto_shadowsocks_save;
    pushButton_proto_shadowsocks_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoShadowsocksSaveVisible(visible);
    };
    LabelFunc label_proto_shadowsocks_info;
    label_proto_shadowsocks_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoShadowsocksInfoVisible(visible);
    };
    label_proto_shadowsocks_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoShadowsocksInfoText(text);
    };
    ProgressFunc progressBar_proto_shadowsocks_reset;
    progressBar_proto_shadowsocks_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoOpenvpnResetVisible(visible);
    };
    progressBar_proto_shadowsocks_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoShadowsocksResetValue(value);
    };
    progressBar_proto_shadowsocks_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoShadowsocksResetValue();
    };
    progressBar_proto_shadowsocks_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoShadowsocksResetMaximium();
    };

    ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_shadowsocks, progressBar_proto_shadowsocks_reset,
    pushButton_proto_shadowsocks_save, label_proto_shadowsocks_info);

    if (!e) {
        m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
    }
    qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
}

void UiLogic::onPushButtonProtoCloakSaveClicked()
{
    QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak);
    protocolConfig = getCloakConfigFromPage(protocolConfig);

    QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
    QJsonObject newContainerConfig = containerConfig;
    newContainerConfig.insert(config_key::cloak, protocolConfig);

    PageFunc page_proto_cloak;
    page_proto_cloak.setEnabledFunc = [this] (bool enabled) -> void {
        setPageProtoCloakEnabled(enabled);
    };
    ButtonFunc pushButton_proto_cloak_save;
    pushButton_proto_cloak_save.setVisibleFunc = [this] (bool visible) ->void {
        setPushButtonProtoCloakSaveVisible(visible);
    };
    LabelFunc label_proto_cloak_info;
    label_proto_cloak_info.setVisibleFunc = [this] (bool visible) ->void {
        setLabelProtoCloakInfoVisible(visible);
    };
    label_proto_cloak_info.setTextFunc = [this] (const QString& text) ->void {
        setLabelProtoCloakInfoText(text);
    };
    ProgressFunc progressBar_proto_cloak_reset;
    progressBar_proto_cloak_reset.setVisibleFunc = [this] (bool visible) ->void {
        setProgressBarProtoCloakResetVisible(visible);
    };
    progressBar_proto_cloak_reset.setValueFunc = [this] (int value) ->void {
        setProgressBarProtoCloakResetValue(value);
    };
    progressBar_proto_cloak_reset.getValueFunc = [this] (void) -> int {
        return getProgressBarProtoCloakResetValue();
    };
    progressBar_proto_cloak_reset.getMaximiumFunc = [this] (void) -> int {
        return getProgressBarProtoCloakResetMaximium();
    };

    ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
        return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
    },
    page_proto_cloak, progressBar_proto_cloak_reset,
    pushButton_proto_cloak_save, label_proto_cloak_info);

    if (!e) {
        m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
        m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
    }

    qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
}

void UiLogic::updateStartPage()
{
    setLineEditStartExistingCodeText("");
    setTextEditNewServerSshKeyText("");
    setLineEditNewServerIpText("");
    setLineEditNewServerPasswordText("");
    setTextEditNewServerSshKeyText("");
    setLineEditNewServerLoginText("");

    setLabelNewServerWaitInfoVisible(false);
    setLabelNewServerWaitInfoText("");
    setProgressBarNewServerConnectionMinimum(0);
    setProgressBarNewServerConnectionMaximum(300);
    setPushButtonNewServerConnectVisible(true);
}



void UiLogic::updateVpnPage()
{
    Settings::RouteMode mode = m_settings.routeMode();
    setRadioButtonVpnModeAllSitesChecked(mode == Settings::VpnAllSites);
    setRadioButtonVpnModeForwardSitesChecked(mode == Settings::VpnOnlyForwardSites);
    setRadioButtonVpnModeExceptSitesChecked(mode == Settings::VpnAllExceptSites);
    setPushButtonVpnAddSiteEnabled(mode != Settings::VpnAllSites);
}




void UiLogic::updateServerPage()
{
    setLabelServerSettingsWaitInfoVisible(false);
    setLabelServerSettingsWaitInfoText("");
    setPushButtonServerSettingsClearVisible(m_settings.haveAuthData(selectedServerIndex));
    setPushButtonServerSettingsClearClientCacheVisible(m_settings.haveAuthData(selectedServerIndex));
    setPushButtonServerSettingsShareFullVisible(m_settings.haveAuthData(selectedServerIndex));
    QJsonObject server = m_settings.server(selectedServerIndex);
    QString port = server.value(config_key::port).toString();
    setLabelServerSettingsServerText(QString("%1@%2%3%4")
                                     .arg(server.value(config_key::userName).toString())
                                     .arg(server.value(config_key::hostName).toString())
                                     .arg(port.isEmpty() ? "" : ":")
                                     .arg(port));
    setLineEditServerSettingsDescriptionText(server.value(config_key::description).toString());
    QString selectedContainerName = m_settings.defaultContainerName(selectedServerIndex);
    setLabelServerSettingsCurrentVpnProtocolText(tr("Protocol: ") + selectedContainerName);
}


void UiLogic::onPushButtonSetupWizardVpnModeFinishClicked()
{
    installServer(getInstallConfigsFromWizardPage());
    if (getCheckBoxSetupWizardVpnModeChecked()) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    } else {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void UiLogic::onPushButtonSetupWizardLowFinishClicked()
{
    installServer(getInstallConfigsFromWizardPage());
}

void UiLogic::onRadioButtonVpnModeAllSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnAllSites);
    }
}

void UiLogic::onRadioButtonVpnModeForwardSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnOnlyForwardSites);
    }
}

void UiLogic::onRadioButtonVpnModeExceptSitesToggled(bool checked)
{
    if (checked) {
        m_settings.setRouteMode(Settings::VpnAllExceptSites);
    }
}



void UiLogic::updateServersListPage()
{
    const QJsonArray &servers = m_settings.serversArray();
    int defaultServer = m_settings.defaultServerIndex();
    std::vector<ServerModelContent> serverListContent;
    for(int i = 0; i < servers.size(); i++) {
        ServerModelContent c;
        auto server = servers.at(i).toObject();
        c.desc = server.value(config_key::description).toString();
        c.address = server.value(config_key::hostName).toString();
        if (c.desc.isEmpty()) {
            c.desc = c.address;
        }
        c.isDefault = (i == defaultServer);
        serverListContent.push_back(c);
    }
    m_serverListModel->setContent(serverListContent);
}

void UiLogic::updateProtocolsPage()
{
    setProgressBarProtocolsContainerReinstallVisible(false);

    auto containers = m_settings.containers(selectedServerIndex);
    DockerContainer defaultContainer = m_settings.defaultContainer(selectedServerIndex);
    bool haveAuthData = m_settings.haveAuthData(selectedServerIndex);

    // all containers
    QList<DockerContainer> allContainers {
        DockerContainer::OpenVpn,
                DockerContainer::OpenVpnOverShadowSocks,
                DockerContainer::OpenVpnOverCloak,
                DockerContainer::WireGuard
    };

    using SetVisibleFunc = std::function<void(bool)>;
    using SetCheckedFunc = std::function<void(bool)>;
    using SetEnabledFunc = std::function<void(bool)>;
    QList<SetCheckedFunc> installButtonsCheckedFunc {
        [this](bool checked) ->void {setPushButtonProtoOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoSsOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoCloakOpenvpnContInstallChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoWireguardContInstallChecked(checked);},
    };
    QList<SetEnabledFunc> installButtonsEnabledFunc {
        [this](bool enabled) ->void {setPushButtonProtoOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoSsOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoCloakOpenvpnContInstallEnabled(enabled);},
        [this](bool enabled) ->void {setPushButtonProtoWireguardContInstallEnabled(enabled);},
    };

    QList<SetCheckedFunc> defaultButtonsCheckedFunc {
        [this](bool checked) ->void {setPushButtonProtoOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoSsOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoCloakOpenvpnContDefaultChecked(checked);},
        [this](bool checked) ->void {setPushButtonProtoWireguardContDefaultChecked(checked);},
    };
    QList<SetVisibleFunc> defaultButtonsVisibleFunc {
        [this](bool visible) ->void {setPushButtonProtoOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoSsOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoCloakOpenvpnContDefaultVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoWireguardContDefaultVisible(visible);},
    };

    QList<SetVisibleFunc> shareButtonsVisibleFunc {
        [this](bool visible) ->void {setPushButtonProtoOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoSsOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoCloakOpenvpnContShareVisible(visible);},
        [this](bool visible) ->void {setPushButtonProtoWireguardContShareVisible(visible);},
    };

    QList<SetVisibleFunc> framesVisibleFunc {
        [this](bool visible) ->void {setFrameOpenvpnSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameOpenvpnSsSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameOpenvpnSsCloakSettingsVisible(visible);},
        [this](bool visible) ->void {setFrameWireguardSettingsVisible(visible);},
    };

    for (int i = 0; i < allContainers.size(); ++i) {
        defaultButtonsCheckedFunc.at(i)(defaultContainer == allContainers.at(i));
        defaultButtonsVisibleFunc.at(i)(haveAuthData && containers.contains(allContainers.at(i)));
        shareButtonsVisibleFunc.at(i)(haveAuthData && containers.contains(allContainers.at(i)));
        installButtonsCheckedFunc.at(i)(containers.contains(allContainers.at(i)));
        installButtonsEnabledFunc.at(i)(haveAuthData);
        framesVisibleFunc.at(i)(containers.contains(allContainers.at(i)));
    }
}

void UiLogic::updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData)
{
    setWidgetProtoOpenvpnEnabled(haveAuthData);
    setPushButtonProtoOpenvpnSaveVisible(haveAuthData);
    setProgressBarProtoOpenvpnResetVisible(haveAuthData);

    setRadioButtonProtoOpenvpnUdpEnabled(true);
    setRadioButtonProtoOpenvpnTcpEnabled(true);

    setLineEditProtoOpenvpnSubnetText(openvpnConfig.value(config_key::subnet_address).
                                      toString(protocols::openvpn::defaultSubnetAddress));

    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
            toString(protocols::openvpn::defaultTransportProto);

    setRadioButtonProtoOpenvpnUdpChecked(trasnsport == protocols::openvpn::defaultTransportProto);
    setRadioButtonProtoOpenvpnTcpChecked(trasnsport != protocols::openvpn::defaultTransportProto);

    setComboBoxProtoOpenvpnCipherText(openvpnConfig.value(config_key::cipher).
                                      toString(protocols::openvpn::defaultCipher));

    setComboBoxProtoOpenvpnHashText(openvpnConfig.value(config_key::hash).
                                    toString(protocols::openvpn::defaultHash));

    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
    setCheckBoxProtoOpenvpnBlockDnsChecked(blockOutsideDns);

    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
    setCheckBoxProtoOpenvpnAutoEncryptionChecked(!isNcpDisabled);

    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
    setCheckBoxProtoOpenvpnTlsAuthChecked(isTlsAuth);

    if (container == DockerContainer::OpenVpnOverShadowSocks) {
        setRadioButtonProtoOpenvpnUdpEnabled(false);
        setRadioButtonProtoOpenvpnTcpEnabled(false);
        setRadioButtonProtoOpenvpnTcpChecked(true);
    }

    setLineEditProtoOpenvpnPortText(openvpnConfig.value(config_key::port).
                                    toString(protocols::openvpn::defaultPort));

    setLineEditProtoOpenvpnPortEnabled(container == DockerContainer::OpenVpn);
}

void UiLogic::updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData)
{
    setWidgetProtoSsEnabled(haveAuthData);
    setPushButtonProtoShadowsocksSaveVisible(haveAuthData);
    setProgressBarProtoShadowsocksResetVisible(haveAuthData);

    setComboBoxProtoShadowsocksCipherText(ssConfig.value(config_key::cipher).
                                          toString(protocols::shadowsocks::defaultCipher));

    setLineEditProtoShadowsocksPortText(ssConfig.value(config_key::port).
                                        toString(protocols::shadowsocks::defaultPort));

    setLineEditProtoShadowsocksPortEnabled(container == DockerContainer::OpenVpnOverShadowSocks);
}

void UiLogic::updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
{
    setWidgetProtoCloakEnabled(haveAuthData);
    setPushButtonProtoCloakSaveVisible(haveAuthData);
    setProgressBarProtoCloakResetVisible(haveAuthData);

    setComboBoxProtoCloakCipherText(ckConfig.value(config_key::cipher).
                                    toString(protocols::cloak::defaultCipher));

    setLineEditProtoCloakSiteText(ckConfig.value(config_key::site).
                                  toString(protocols::cloak::defaultRedirSite));

    setLineEditProtoCloakPortText(ckConfig.value(config_key::port).
                                  toString(protocols::cloak::defaultPort));

    setLineEditProtoCloakPortEnabled(container == DockerContainer::OpenVpnOverCloak);
}

void UiLogic::updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                                DockerContainer container)
{
    selectedDockerContainer = container;
    selectedServerIndex = serverIndex;

    //const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

    setPageShareAmneziaVisible(false);
    setPageShareOpenvpnVisible(false);
    setPageShareShadowsocksVisible(false);
    setPageShareCloakVisible(false);
    setPageShareFullAccessVisible(false);

    enum currentWidget {
        full_access = 0,
        share_amezia,
        share_openvpn,
        share_shadowshock,
        share_cloak
    };

    if (container == DockerContainer::OpenVpn) {
        setPageShareAmneziaVisible(true);
        setPageShareOpenvpnVisible(true);

        QString cfg = tr("Press Generate config");
        setTextEditShareOpenvpnCodeText(cfg);
        setPushButtonShareOpenvpnCopyEnabled(false);
        setPushButtonShareOpenvpnSaveEnabled(false);

        setToolBoxShareConnectionCurrentIndex(share_openvpn);
    }

    if (container == DockerContainer::OpenVpnOverShadowSocks ||
            container == DockerContainer::OpenVpnOverCloak) {
        setPageShareAmneziaVisible(true);
        setPageShareShadowsocksVisible(true);

        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::ShadowSocks);
        QString cfg = protoConfig.value(config_key::last_config).toString();

        if (cfg.isEmpty()) {
            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

            ErrorCode e = ErrorCode::NoError;
            cfg = ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, &e);

            setPushButtonShareSsCopyEnabled(true);
        }

        QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

        QString ssString = QString("%1:%2@%3:%4")
                .arg(ssConfig.value("method").toString())
                .arg(ssConfig.value("password").toString())
                .arg(ssConfig.value("server").toString())
                .arg(ssConfig.value("server_port").toString());

        ssString = "ss://" + ssString.toUtf8().toBase64();
        setLineEditShareSsStringText(ssString);
        updateQRCodeImage(ssString, [this](const QString& labelText) ->void {
            setLabelShareSsQrCodeText(labelText);
        });

        setLabelShareSsServerText(ssConfig.value("server").toString());
        setLabelShareSsPortText(ssConfig.value("server_port").toString());
        setLabelShareSsMethodText(ssConfig.value("method").toString());
        setLabelShareSsPasswordText(ssConfig.value("password").toString());

        setToolBoxShareConnectionCurrentIndex(share_shadowshock);
    }

    if (container == DockerContainer::OpenVpnOverCloak) {
        //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
        setPageShareCloakVisible(true);
        setPlainTextEditShareCloakText(QString(""));

        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::Cloak);
        QString cfg = protoConfig.value(config_key::last_config).toString();

        if (cfg.isEmpty()) {
            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

            ErrorCode e = ErrorCode::NoError;
            cfg = CloakConfigurator::genCloakConfig(credentials, container, containerConfig, &e);

            setPushButtonShareCloakCopyEnabled(true);
        }

        QJsonObject cloakConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();
        cloakConfig.remove(config_key::transport_proto);
        cloakConfig.insert("ProxyMethod", "shadowsocks");

        setPlainTextEditShareCloakText(QJsonDocument(cloakConfig).toJson());
    }

    // Full access
    if (container == DockerContainer::None) {
        setPageShareFullAccessVisible(true);

        const QJsonObject &server = m_settings.server(selectedServerIndex);

        QByteArray ba = QJsonDocument(server).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

        setTextEditShareFullCodeText(QString("vpn://%1").arg(QString(ba)));
        setToolBoxShareConnectionCurrentIndex(full_access);
    }

    //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));

    // Amnezia sharing
    //    QJsonObject exportContainer;
    //    for (Protocol p: protocolsForContainer(container)) {
    //        QJsonObject protocolConfig = containerConfig.value(protoToString(p)).toObject();
    //        protocolConfig.remove(config_key::last_config);
    //        exportContainer.insert(protoToString(p), protocolConfig);
    //    }
    //    exportContainer.insert(config_key::container, containerToString(container));

    //    ui->textEdit_share_amnezia_code->setPlainText(QJsonDocument(exportContainer).toJson());

    setTextEditShareAmneziaCodeText(tr(""));
}


void UiLogic::updateQRCodeImage(const QString &text, const std::function<void(const QString&)>& setLabelFunc)
{
    int levelIndex = 1;
    int versionIndex = 0;
    bool bExtent = true;
    int maskIndex = -1;

    m_qrEncode.EncodeData( levelIndex, versionIndex, bExtent, maskIndex, text.toUtf8().data() );

    int qrImageSize = m_qrEncode.m_nSymbleSize;

    int encodeImageSize = qrImageSize + ( QR_MARGIN * 2 );
    QImage encodeImage( encodeImageSize, encodeImageSize, QImage::Format_Mono );

    encodeImage.fill( 1 );

    for ( int i = 0; i < qrImageSize; i++ )
        for ( int j = 0; j < qrImageSize; j++ )
            if ( m_qrEncode.m_byModuleData[i][j] )
                encodeImage.setPixel( i + QR_MARGIN, j + QR_MARGIN, 0 );

    QByteArray byteArray;
    QBuffer buffer(&byteArray);
    encodeImage.save(&buffer, "PNG"); // writes the image in PNG format inside the buffer
    QString iconBase64 = QString::fromLatin1(byteArray.toBase64().data());

    setLabelFunc(iconBase64);
}

QJsonObject UiLogic::getOpenVpnConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::subnet_address, getLineEditProtoOpenvpnSubnetText());
    oldConfig.insert(config_key::transport_proto, getRadioButtonProtoOpenvpnUdpChecked() ? protocols::UDP : protocols::TCP);
    oldConfig.insert(config_key::ncp_disable, ! getCheckBoxProtoOpenvpnAutoEncryptionChecked());
    oldConfig.insert(config_key::cipher, getComboBoxProtoOpenvpnCipherText());
    oldConfig.insert(config_key::hash, getComboBoxProtoOpenvpnHashText());
    oldConfig.insert(config_key::block_outside_dns, getCheckBoxProtoOpenvpnBlockDnsChecked());
    oldConfig.insert(config_key::port, getLineEditProtoOpenvpnPortText());
    oldConfig.insert(config_key::tls_auth, getCheckBoxProtoOpenvpnTlsAuthChecked());
    return oldConfig;
}

QJsonObject UiLogic::getShadowSocksConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, getComboBoxProtoShadowsocksCipherText());
    oldConfig.insert(config_key::port, getLineEditProtoShadowsocksPortText());

    return oldConfig;
}

QJsonObject UiLogic::getCloakConfigFromPage(QJsonObject oldConfig)
{
    oldConfig.insert(config_key::cipher, getComboBoxProtoCloakCipherText());
    oldConfig.insert(config_key::site, getLineEditProtoCloakSiteText());
    oldConfig.insert(config_key::port, getLineEditProtoCloakPortText());

    return oldConfig;
}
