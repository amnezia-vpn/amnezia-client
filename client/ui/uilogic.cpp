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
#include "ui/server_widget.h"
#include "ui_server_widget.h"

#if defined Q_OS_MAC || defined Q_OS_LINUX
#include "ui/macos_util.h"
#endif

using namespace amnezia;

UiLogic::UiLogic(QObject *parent) :
    QObject(parent),
    m_frameWireguardSettingsVisible{false},
    m_frameFireguardVisible{false},
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
    m_comboBoxNewServerSsCipherText{},
    m_lineEditNewServerOpenvpnPortText{},
    m_comboBoxNewServerOpenvpnProtoText{},
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
    m_checkBoxAppSettingsAutostartChecked{false},
    m_checkBoxAppSettingsAutoconnectChecked{false},
    m_checkBoxAppSettingsStartMinimizedChecked{false},
    m_lineEditNetworkSettingsDns1Text{},
    m_lineEditNetworkSettingsDns2Text{},
    m_labelAppSettingsVersionText{},
    m_pushButtonGeneralSettingsShareConnectionEnable{},
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
    m_ipAddressValidatorRegex{Utils::ipAddressRegExp().pattern()},
    m_pushButtonConnectChecked{false},
    m_vpnConnection(nullptr)
{
    m_vpnConnection = new VpnConnection(this);
    //    connect(m_vpnConnection, SIGNAL(bytesChanged(quint64, quint64)), this, SLOT(onBytesChanged(quint64, quint64)));
    //    connect(m_vpnConnection, SIGNAL(connectionStateChanged(VpnProtocol::ConnectionState)), this, SLOT(onConnectionStateChanged(VpnProtocol::ConnectionState)));
    //    connect(m_vpnConnection, SIGNAL(vpnProtocolError(amnezia::ErrorCode)), this, SLOT(onVpnProtocolError(amnezia::ErrorCode)));
}

void UiLogic::initalizeUiLogic()
{
    setFrameWireguardSettingsVisible(false);
    setFrameFireguardVisible(false);
    setFrameNewServerSettingsParentWireguardVisible(false);

    setupTray();
    setupNewServerConnections();
    //    setupSitesPageConnections();
    //        setupGeneralSettingsConnections();
    //    setupProtocolsPageConnections();
    setupNewServerPageConnections();
    //    setupSharePageConnections();
    //    setupServerSettingsPageConnections();

    //    ui->label_error_text->clear();
    //    installEventFilter(this);
    //    ui->widget_tittlebar->installEventFilter(this);

    //    ui->stackedWidget_main->setSpeed(200);
    //    ui->stackedWidget_main->setAnimation(QEasingCurve::Linear);


    //    ui->tableView_sites->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    //    //    ui->tableView_sites->setColumnWidth(0, 450);
    //    //    ui->tableView_sites->setColumnWidth(1, 120);

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

    //    // Post initialization
    //    goToPage(Page::Start, true, false);

    //    if (m_settings.defaultServerIndex() >= 0 && m_settings.serversCount() > 0) {
    //        goToPage(Page::Vpn, true, false);
    //    }

    //    //ui->pushButton_general_settings_exit->hide();
    //    updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);

    //    setFixedSize(width(),height());

    //    qInfo().noquote() << QString("Started %1 version %2").arg(APPLICATION_NAME).arg(APP_VERSION);
    //    qInfo().noquote() << QString("%1 (%2)").arg(QSysInfo::prettyProductName()).arg(QSysInfo::currentCpuArchitecture());



    //    onConnectionStateChanged(VpnProtocol::Disconnected);

    //    if (m_settings.isAutoConnect() && m_settings.defaultServerIndex() >= 0) {
    //        QTimer::singleShot(1000, this, [this](){
    //            ui->pushButton_connect->setEnabled(false);
    //            onConnect();
    //        });
    //    }

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

    //    //ui->toolBox_share_connection->removeItem(ui->toolBox_share_connection->indexOf(ui->page_share_shadowsocks));
    //    //ui->page_share_shadowsocks->setVisible(false);


    //    sitesModels.insert(Settings::VpnOnlyForwardSites, new SitesModel(Settings::VpnOnlyForwardSites));
    //    sitesModels.insert(Settings::VpnAllExceptSites, new SitesModel(Settings::VpnAllExceptSites));
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

bool UiLogic::getFrameFireguardVisible() const
{
    return m_frameFireguardVisible;
}

void UiLogic::setFrameFireguardVisible(bool frameFireguardVisible)
{
    if (m_frameFireguardVisible != frameFireguardVisible) {
        m_frameFireguardVisible = frameFireguardVisible;
        emit frameFireguardVisibleChanged();
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

void UiLogic::setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked)
{
    if (m_radioButtonVpnModeAllSitesChecked != radioButtonVpnModeAllSitesChecked) {
        m_radioButtonVpnModeAllSitesChecked = radioButtonVpnModeAllSitesChecked;
        emit radioButtonVpnModeAllSitesCheckedChanged();
    }
}

bool UiLogic::getRadioButtonVpnModeForwardSitesChecked() const
{
    return m_radioButtonVpnModeForwardSitesChecked;
}

void UiLogic::setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked)
{
    if (m_radioButtonVpnModeForwardSitesChecked != radioButtonVpnModeForwardSitesChecked) {
        m_radioButtonVpnModeForwardSitesChecked = radioButtonVpnModeForwardSitesChecked;
        emit radioButtonVpnModeForwardSitesCheckedChanged();
    }
}

bool UiLogic::getRadioButtonVpnModeExceptSitesChecked() const
{
    return m_radioButtonVpnModeExceptSitesChecked;
}

void UiLogic::setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked)
{
    if (m_radioButtonVpnModeExceptSitesChecked != radioButtonVpnModeExceptSitesChecked) {
        m_radioButtonVpnModeExceptSitesChecked = radioButtonVpnModeExceptSitesChecked;
        emit radioButtonVpnModeExceptSitesCheckedChanged();
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

bool UiLogic::getCheckBoxAppSettingsAutostartChecked() const
{
    return m_checkBoxAppSettingsAutostartChecked;
}

void UiLogic::setCheckBoxAppSettingsAutostartChecked(bool checkBoxAppSettingsAutostartChecked)
{
    if (m_checkBoxAppSettingsAutostartChecked != checkBoxAppSettingsAutostartChecked) {
        m_checkBoxAppSettingsAutostartChecked = checkBoxAppSettingsAutostartChecked;
        emit checkBoxAppSettingsAutostartCheckedChanged();
    }
}

bool UiLogic::getCheckBoxAppSettingsAutoconnectChecked() const
{
    return m_checkBoxAppSettingsAutoconnectChecked;
}

void UiLogic::setCheckBoxAppSettingsAutoconnectChecked(bool checkBoxAppSettingsAutoconnectChecked)
{
    if (m_checkBoxAppSettingsAutoconnectChecked != checkBoxAppSettingsAutoconnectChecked) {
        m_checkBoxAppSettingsAutoconnectChecked = checkBoxAppSettingsAutoconnectChecked;
        emit checkBoxAppSettingsAutoconnectCheckedChanged();
    }
}

bool UiLogic::getCheckBoxAppSettingsStartMinimizedChecked() const
{
    return m_checkBoxAppSettingsStartMinimizedChecked;
}

void UiLogic::setCheckBoxAppSettingsStartMinimizedChecked(bool checkBoxAppSettingsStartMinimizedChecked)
{
    if (m_checkBoxAppSettingsStartMinimizedChecked != checkBoxAppSettingsStartMinimizedChecked) {
        m_checkBoxAppSettingsStartMinimizedChecked = checkBoxAppSettingsStartMinimizedChecked;
        emit checkBoxAppSettingsStartMinimizedCheckedChanged();
    }
}

QString UiLogic::getLineEditNetworkSettingsDns1Text() const
{
    return m_lineEditNetworkSettingsDns1Text;
}

void UiLogic::setLineEditNetworkSettingsDns1Text(const QString &lineEditNetworkSettingsDns1Text)
{
    if (m_lineEditNetworkSettingsDns1Text != lineEditNetworkSettingsDns1Text) {
        m_lineEditNetworkSettingsDns1Text = lineEditNetworkSettingsDns1Text;
        emit lineEditNetworkSettingsDns1TextChanged();
    }
}

QString UiLogic::getLineEditNetworkSettingsDns2Text() const
{
    return m_lineEditNetworkSettingsDns2Text;
}

void UiLogic::setLineEditNetworkSettingsDns2Text(const QString &lineEditNetworkSettingsDns2Text)
{
    if (m_lineEditNetworkSettingsDns2Text != lineEditNetworkSettingsDns2Text) {
        m_lineEditNetworkSettingsDns2Text = lineEditNetworkSettingsDns2Text;
        emit lineEditNetworkSettingsDns2TextChanged();
    }
}

QString UiLogic::getLabelAppSettingsVersionText() const
{
    return m_labelAppSettingsVersionText;
}

void UiLogic::setLabelAppSettingsVersionText(const QString &labelAppSettingsVersionText)
{
    if (m_labelAppSettingsVersionText != labelAppSettingsVersionText) {
        m_labelAppSettingsVersionText = labelAppSettingsVersionText;
        emit labelAppSettingsVersionTextChanged();
    }
}

bool UiLogic::getPushButtonGeneralSettingsShareConnectionEnable() const
{
    return m_pushButtonGeneralSettingsShareConnectionEnable;
}

void UiLogic::setPushButtonGeneralSettingsShareConnectionEnable(bool pushButtonGeneralSettingsShareConnectionEnable)
{
    if (m_pushButtonGeneralSettingsShareConnectionEnable != pushButtonGeneralSettingsShareConnectionEnable) {
        m_pushButtonGeneralSettingsShareConnectionEnable = pushButtonGeneralSettingsShareConnectionEnable;
        emit pushButtonGeneralSettingsShareConnectionEnableChanged();
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

QString UiLogic::getIpAddressValidatorRegex() const
{
    return m_ipAddressValidatorRegex;
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

//UiLogic::~UiLogic()
//{
//    hide();

//    m_vpnConnection->disconnectFromVpn();
//    for (int i = 0; i < 50; i++) {
//        qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
//        QThread::msleep(100);
//        if (m_vpnConnection->isDisconnected()) {
//            break;
//        }
//    }

//    delete m_vpnConnection;
//    delete ui;

//    qDebug() << "Application closed";
//}

//void UiLogic::showOnStartup()
//{
//    if (! m_settings.isStartMinimized()) show();
//    else {
//#if defined Q_OS_MACX
//        setDockIconVisible(false);
//#endif
//    }
//}

//bool UiLogic::eventFilter(QObject *obj, QEvent *event)
//{
//    if (obj == ui->widget_tittlebar) {
//        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

//        if (!mouseEvent)
//            return false;

//        if(event->type() == QEvent::MouseButtonPress) {
//            offset = mouseEvent->pos();
//            canMove = true;
//        }

//        if(event->type() == QEvent::MouseButtonRelease) {
//            canMove = false;
//        }

//        if (event->type() == QEvent::MouseMove) {
//            if(canMove && (mouseEvent->buttons() & Qt::LeftButton)) {
//                move(mapToParent(mouseEvent->pos() - offset));
//            }

//            event->ignore();
//            return false;
//        }
//    }

//    return QUiLogic::eventFilter(obj, event);
//}

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

//void UiLogic::closeEvent(QCloseEvent *event)
//{
//    if (m_settings.serversCount() == 0) qApp->quit();
//    else {
//        hide();
//        event->ignore();
//    }
//}

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
//    if (ui->pushButton_new_server_connect_key->isChecked()){
//        if (ui->lineEdit_new_server_ip->text().isEmpty() ||
//                ui->lineEdit_new_server_login->text().isEmpty() ||
//                ui->textEdit_new_server_ssh_key->toPlainText().isEmpty() ) {

//            ui->label_new_server_wait_info->setText(tr("Please fill in all fields"));
//            return;
//        }
//    }
//    else {
//        if (ui->lineEdit_new_server_ip->text().isEmpty() ||
//                ui->lineEdit_new_server_login->text().isEmpty() ||
//                ui->lineEdit_new_server_password->text().isEmpty() ) {

//            ui->label_new_server_wait_info->setText(tr("Please fill in all fields"));
//            return;
//        }
//    }


//    qDebug() << "UiLogic::onPushButtonNewServerConnect checking new server";


//    ServerCredentials serverCredentials;
//    serverCredentials.hostName = ui->lineEdit_new_server_ip->text();
//    if (serverCredentials.hostName.contains(":")) {
//        serverCredentials.port = serverCredentials.hostName.split(":").at(1).toInt();
//        serverCredentials.hostName = serverCredentials.hostName.split(":").at(0);
//    }
//    serverCredentials.userName = ui->lineEdit_new_server_login->text();
//    if (ui->pushButton_new_server_connect_key->isChecked()){
//        QString key = ui->textEdit_new_server_ssh_key->toPlainText();
//        if (key.startsWith("ssh-rsa")) {
//            QMessageBox::warning(this, APPLICATION_NAME,
//                                 tr("It's public key. Private key required"));

//            return;
//        }

//        if (key.contains("OPENSSH") && key.contains("BEGIN") && key.contains("PRIVATE KEY")) {
//            key = SshConfigurator::convertOpenSShKey(key);
//        }

//        serverCredentials.password = key;
//    }
//    else {
//        serverCredentials.password = ui->lineEdit_new_server_password->text();
//    }

//    ui->pushButton_new_server_connect->setEnabled(false);
//    ui->pushButton_new_server_connect->setText(tr("Connecting..."));

//    ErrorCode e = ErrorCode::NoError;
//#ifdef Q_DEBUG
//    //QString output = ServerController::checkSshConnection(serverCredentials, &e);
//#else
//    QString output;
//#endif

//    bool ok = true;
//    if (e) {
//        ui->label_new_server_wait_info->show();
//        ui->label_new_server_wait_info->setText(errorString(e));
//        ok = false;
//    }
//    else {
//        if (output.contains("Please login as the user")) {
//            output.replace("\n", "");
//            ui->label_new_server_wait_info->show();
//            ui->label_new_server_wait_info->setText(output);
//            ok = false;
//        }
//    }

//    ui->pushButton_new_server_connect->setEnabled(true);
//    ui->pushButton_new_server_connect->setText(tr("Connect"));

//    installCredentials = serverCredentials;
//    if (ok) goToPage(Page::NewServer);
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
    //    if (containers.isEmpty()) return;

    //    goToPage(Page::ServerConfiguring);
    //    QEventLoop loop;
    //    QTimer::singleShot(500, &loop, SLOT(quit()));
    //    loop.exec();
    //    qApp->processEvents();

    //    bool ok = installContainers(installCredentials, containers,
    //                                ui->page_new_server_configuring,
    //                                ui->progressBar_new_server_configuring,
    //                                nullptr,
    //                                ui->label_new_server_configuring_wait_info);

    //    if (ok) {
    //        QJsonObject server;
    //        server.insert(config_key::hostName, installCredentials.hostName);
    //        server.insert(config_key::userName, installCredentials.userName);
    //        server.insert(config_key::password, installCredentials.password);
    //        server.insert(config_key::port, installCredentials.port);
    //        server.insert(config_key::description, m_settings.nextAvailableServerName());

    //        QJsonArray containerConfigs;
    //        for (const QJsonObject &cfg : containers) {
    //            containerConfigs.append(cfg);
    //        }
    //        server.insert(config_key::containers, containerConfigs);
    //        server.insert(config_key::defaultContainer, containerToString(containers.firstKey()));

    //        m_settings.addServer(server);
    //        m_settings.setDefaultServer(m_settings.serversCount() - 1);

    //        setStartPage(Page::Vpn);
    //        qApp->processEvents();
    //    }
    //    else {
    //        closePage();
    //    }
}

void UiLogic::onPushButtonNewServerImport()
{
//    QString s = ui->lineEdit_start_existing_code->text();
//    s.replace("vpn://", "");
//    QJsonObject o = QJsonDocument::fromJson(QByteArray::fromBase64(s.toUtf8(), QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals)).object();

//    ServerCredentials credentials;
//    credentials.hostName = o.value("h").toString();
//    if (credentials.hostName.isEmpty()) credentials.hostName = o.value(config_key::hostName).toString();

//    credentials.port = o.value("p").toInt();
//    if (credentials.port == 0) credentials.port = o.value(config_key::port).toInt();

//    credentials.userName = o.value("u").toString();
//    if (credentials.userName.isEmpty()) credentials.userName = o.value(config_key::userName).toString();

//    credentials.password = o.value("w").toString();
//    if (credentials.password.isEmpty()) credentials.password = o.value(config_key::password).toString();

//    if (credentials.isValid()) {
//        o.insert(config_key::hostName, credentials.hostName);
//        o.insert(config_key::port, credentials.port);
//        o.insert(config_key::userName, credentials.userName);
//        o.insert(config_key::password, credentials.password);

//        o.remove("h");
//        o.remove("p");
//        o.remove("u");
//        o.remove("w");
//    }
//    qDebug() << QString("Added server %3@%1:%2").
//                arg(credentials.hostName).
//                arg(credentials.port).
//                arg(credentials.userName);

//    //qDebug() << QString("Password") << credentials.password;

//    if (credentials.isValid() || o.contains(config_key::containers)) {
//        m_settings.addServer(o);
//        m_settings.setDefaultServer(m_settings.serversCount() - 1);

//        setStartPage(Page::Vpn);
//    }
//    else {
//        qDebug() << "Failed to import profile";
//        qDebug().noquote() << QJsonDocument(o).toJson();
//        return;
//    }

//    if (!o.contains(config_key::containers)) {
//        selectedServerIndex = m_settings.defaultServerIndex();
//        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);
//        goToPage(Page::ServerVpnProtocols);
//    }
}

//bool UiLogic::installContainers(ServerCredentials credentials,
//                                const QMap<DockerContainer, QJsonObject> &containers,
//                                QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
//{
//    if (!progress) return false;

//    if (page) page->setEnabled(false);
//    if (button) button->setVisible(false);

//    if (info) info->setVisible(true);
//    if (info) info->setText(tr("Please wait, configuring process may take up to 5 minutes"));


//    int cnt = 0;
//    for (QMap<DockerContainer, QJsonObject>::const_iterator i = containers.constBegin(); i != containers.constEnd(); i++, cnt++) {
//        QTimer timer;
//        connect(&timer, &QTimer::timeout, [progress](){
//            progress->setValue(progress->value() + 1);
//        });

//        progress->setValue(0);
//        timer.start(1000);

//        progress->setTextVisible(true);
//        progress->setFormat(QString("Installing %1 %2 %3").arg(cnt+1).arg(tr("of")).arg(containers.size()));

//        ErrorCode e = ServerController::setupContainer(credentials, i.key(), i.value());
//        qDebug() << "Setup server finished with code" << e;
//        ServerController::disconnectFromHost(credentials);

//        if (e) {
//            if (page) page->setEnabled(true);
//            if (button) button->setVisible(true);
//            if (info) info->setVisible(false);

//            QMessageBox::warning(this, APPLICATION_NAME,
//                                 tr("Error occurred while configuring server.") + "\n" +
//                                 errorString(e));

//            return false;
//        }

//        // just ui progressbar tweak
//        timer.stop();

//        int remaining_val = progress->maximum() - progress->value();

//        if (remaining_val > 0) {
//            QTimer timer1;
//            QEventLoop loop1;

//            connect(&timer1, &QTimer::timeout, [&](){
//                progress->setValue(progress->value() + 1);
//                if (progress->value() >= progress->maximum()) {
//                    loop1.quit();
//                }
//            });

//            timer1.start(5);
//            loop1.exec();
//        }
//    }


//    if (button) button->show();
//    if (page) page->setEnabled(true);
//    if (info) info->setText(tr("Amnezia server installed"));

//    return true;
//}

//ErrorCode UiLogic::doInstallAction(const std::function<ErrorCode()> &action, QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info)
//{
//    progress->show();
//    if (page) page->setEnabled(false);
//    if (button) button->setVisible(false);

//    if (info) info->setVisible(true);
//    if (info) info->setText(tr("Please wait, configuring process may take up to 5 minutes"));


//    QTimer timer;
//    connect(&timer, &QTimer::timeout, [progress](){
//        progress->setValue(progress->value() + 1);
//    });

//    progress->setValue(0);
//    timer.start(1000);

//    ErrorCode e = action();
//    qDebug() << "doInstallAction finished with code" << e;

//    if (e) {
//        if (page) page->setEnabled(true);
//        if (button) button->setVisible(true);
//        if (info) info->setVisible(false);

//        QMessageBox::warning(this, APPLICATION_NAME,
//                             tr("Error occurred while configuring server.") + "\n" +
//                             errorString(e));

//        progress->hide();
//        return e;
//    }

//    // just ui progressbar tweak
//    timer.stop();

//    int remaining_val = progress->maximum() - progress->value();

//    if (remaining_val > 0) {
//        QTimer timer1;
//        QEventLoop loop1;

//        connect(&timer1, &QTimer::timeout, [&](){
//            progress->setValue(progress->value() + 1);
//            if (progress->value() >= progress->maximum()) {
//                loop1.quit();
//            }
//        });

//        timer1.start(5);
//        loop1.exec();
//    }


//    progress->hide();
//    if (button) button->show();
//    if (page) page->setEnabled(true);
//    if (info) info->setText(tr("Operation finished"));

//    return ErrorCode::NoError;
//}

//void UiLogic::onPushButtonClearServer(bool)
//{
//    ui->page_server_settings->setEnabled(false);
//    ui->pushButton_server_settings_clear->setText(tr("Uninstalling Amnezia software..."));

//    if (m_settings.defaultServerIndex() == selectedServerIndex) {
//        onDisconnect();
//    }

//    ErrorCode e = ServerController::removeAllContainers(m_settings.serverCredentials(selectedServerIndex));
//    ServerController::disconnectFromHost(m_settings.serverCredentials(selectedServerIndex));
//    if (e) {
//        QMessageBox::warning(this, APPLICATION_NAME,
//                             tr("Error occurred while configuring server.") + "\n" +
//                             errorString(e) + "\n" +
//                             tr("See logs for details."));

//    }
//    else {
//        ui->label_server_settings_wait_info->show();
//        ui->label_server_settings_wait_info->setText(tr("Amnezia server successfully uninstalled"));
//    }

//    m_settings.setContainers(selectedServerIndex, {});
//    m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);

//    ui->page_server_settings->setEnabled(true);
//    ui->pushButton_server_settings_clear->setText(tr("Clear server from Amnezia software"));
//}

//void UiLogic::onPushButtonForgetServer(bool)
//{
//    if (m_settings.defaultServerIndex() == selectedServerIndex && m_vpnConnection->isConnected()) {
//        onDisconnect();
//    }
//    m_settings.removeServer(selectedServerIndex);

//    if (m_settings.defaultServerIndex() == selectedServerIndex) {
//        m_settings.setDefaultServer(0);
//    }
//    else if (m_settings.defaultServerIndex() > selectedServerIndex) {
//        m_settings.setDefaultServer(m_settings.defaultServerIndex() - 1);
//    }

//    if (m_settings.serversCount() == 0) {
//        m_settings.setDefaultServer(-1);
//    }


//    selectedServerIndex = -1;

//    updateServersListPage();

//    if (m_settings.serversCount() == 0) {
//        setStartPage(Page::Start);
//    }
//    else {
//        closePage();
//    }
//}

//void UiLogic::onBytesChanged(quint64 receivedData, quint64 sentData)
//{
//    ui->label_speed_received->setText(VpnConnection::bytesPerSecToText(receivedData));
//    ui->label_speed_sent->setText(VpnConnection::bytesPerSecToText(sentData));
//}

//void UiLogic::onConnectionStateChanged(VpnProtocol::ConnectionState state)
//{
//    qDebug() << "UiLogic::onConnectionStateChanged" << VpnProtocol::textConnectionState(state);

//    bool pushButtonConnectEnabled = false;
//    bool radioButtonsModeEnabled = false;
//    ui->label_state->setText(VpnProtocol::textConnectionState(state));

//    setTrayState(state);

//    switch (state) {
//    case VpnProtocol::Disconnected:
//        onBytesChanged(0,0);
//        ui->pushButton_connect->setChecked(false);
//        pushButtonConnectEnabled = true;
//        radioButtonsModeEnabled = true;
//        break;
//    case VpnProtocol::Preparing:
//        pushButtonConnectEnabled = false;
//        radioButtonsModeEnabled = false;
//        break;
//    case VpnProtocol::Connecting:
//        pushButtonConnectEnabled = false;
//        radioButtonsModeEnabled = false;
//        break;
//    case VpnProtocol::Connected:
//        pushButtonConnectEnabled = true;
//        radioButtonsModeEnabled = false;
//        break;
//    case VpnProtocol::Disconnecting:
//        pushButtonConnectEnabled = false;
//        radioButtonsModeEnabled = false;
//        break;
//    case VpnProtocol::Reconnecting:
//        pushButtonConnectEnabled = true;
//        radioButtonsModeEnabled = false;
//        break;
//    case VpnProtocol::Error:
//        ui->pushButton_connect->setChecked(false);
//        pushButtonConnectEnabled = true;
//        radioButtonsModeEnabled = true;
//        break;
//    case VpnProtocol::Unknown:
//        pushButtonConnectEnabled = true;
//        radioButtonsModeEnabled = true;
//    }

//    ui->pushButton_connect->setEnabled(pushButtonConnectEnabled);
//    ui->widget_vpn_mode->setEnabled(radioButtonsModeEnabled);
//}

//void UiLogic::onVpnProtocolError(ErrorCode errorCode)
//{
//    ui->label_error_text->setText(errorString(errorCode));
//}

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

UiLogic::Page UiLogic::currentPage()
{
    return static_cast<UiLogic::Page>(getCurrentPageValue());
}

void UiLogic::setupNewServerConnections()
{
    connect(this, &UiLogic::pushButtonNewServerConnectConfigureClicked, this, [this](){
        installServer(getInstallConfigsFromProtocolsPage());
    });
}


//void UiLogic::setupSitesPageConnections()
//{
//    connect(ui->pushButton_sites_add_custom, &QPushButton::clicked, this, [this](){ onPushButtonAddCustomSitesClicked(); });

//    connect(ui->lineEdit_sites_add_custom, &QLineEdit::returnPressed, [&](){
//        ui->pushButton_sites_add_custom->click();
//    });

//    connect(ui->pushButton_sites_delete, &QPushButton::clicked, this, [this](){
//        Settings::RouteMode mode = m_settings.routeMode();

//        QItemSelectionModel* selection = ui->tableView_sites->selectionModel();
//        if (!selection) return;

//        {
//            QModelIndexList indexesSites = selection->selectedRows(0);

//            QStringList sites;
//            for (const QModelIndex &index : indexesSites) {
//                sites.append(index.data().toString());
//            }

//            m_settings.removeVpnSites(mode, sites);
//        }

//        if (m_vpnConnection->connectionState() == VpnProtocol::Connected) {
//            QModelIndexList indexesIps = selection->selectedRows(1);

//            QStringList ips;
//            for (const QModelIndex &index : indexesIps) {
//                if (index.data().toString().isEmpty()) {
//                    ips.append(index.sibling(index.row(), 0).data().toString());
//                }
//                else {
//                    ips.append(index.data().toString());
//                }
//            }

//            m_vpnConnection->deleteRoutes(ips);
//            m_vpnConnection->flushDns();
//        }

//        updateSitesPage();
//    });

//    connect(ui->pushButton_sites_import, &QPushButton::clicked, this, [this](){
//        QString fileName = QFileDialog::getOpenFileName(this, tr("Import IP addresses"),
//                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));

//        QFile file(fileName);
//        if (!file.open(QIODevice::ReadOnly)) return;

//        Settings::RouteMode mode = m_settings.routeMode();

//        QStringList ips;
//        while (!file.atEnd()) {
//            QString line = file.readLine();

//            int pos = 0;
//            QRegExp rx = Utils::ipAddressWithSubnetRegExp();
//            while ((pos = rx.indexIn(line, pos)) != -1) {
//                ips << rx.cap(0);
//                pos += rx.matchedLength();
//            }
//        }

//        m_settings.addVpnIps(mode, ips);

//        m_vpnConnection->addRoutes(QStringList() << ips);
//        m_vpnConnection->flushDns();

//        updateSitesPage();
//    });
//}


//void UiLogic::setupGeneralSettingsConnections()
//{
//    connect(ui->pushButton_general_settings_exit, &QPushButton::clicked, this, [&](){ qApp->quit(); });

//    connect(ui->pushButton_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::GeneralSettings); });
//    connect(ui->pushButton_general_settings_app_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::AppSettings); });
//    connect(ui->pushButton_general_settings_network_settings, &QPushButton::clicked, this, [this](){ goToPage(Page::NetworkSettings); });
//    connect(ui->pushButton_general_settings_server_settings, &QPushButton::clicked, this, [this](){
//        selectedServerIndex = m_settings.defaultServerIndex();
//        goToPage(Page::ServerSettings);
//    });
//    connect(ui->pushButton_general_settings_servers_list, &QPushButton::clicked, this, [this](){ goToPage(Page::ServersList); });
//    connect(ui->pushButton_general_settings_share_connection, &QPushButton::clicked, this, [this](){
//        selectedServerIndex = m_settings.defaultServerIndex();
//        selectedDockerContainer = m_settings.defaultContainer(selectedServerIndex);

//        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer);
//        goToPage(Page::ShareConnection);
//    });

//    connect(ui->pushButton_general_settings_add_server, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });

//}


//void UiLogic::setupProtocolsPageConnections()
//{
//    QJsonObject openvpnConfig;

//    // all containers
//    QList<DockerContainer> containers {
//        DockerContainer::OpenVpn,
//                DockerContainer::OpenVpnOverShadowSocks,
//                DockerContainer::OpenVpnOverCloak,
//                DockerContainer::WireGuard
//    };

//    // default buttons
//    QList<QPushButton *> defaultButtons {
//        ui->pushButton_proto_openvpn_cont_default,
//                ui->pushButton_proto_ss_openvpn_cont_default,
//                ui->pushButton_proto_cloak_openvpn_cont_default,
//                ui->pushButton_proto_wireguard_cont_default
//    };

//    for (int i = 0; i < containers.size(); ++i) {
//        connect(defaultButtons.at(i), &QPushButton::clicked, this, [this, containers, i](){
//            m_settings.setDefaultContainer(selectedServerIndex, containers.at(i));
//            updateProtocolsPage();
//        });
//    }

//    // install buttons
//    QList<QPushButton *> installButtons {
//        ui->pushButton_proto_openvpn_cont_install,
//                ui->pushButton_proto_ss_openvpn_cont_install,
//                ui->pushButton_proto_cloak_openvpn_cont_install,
//                ui->pushButton_proto_wireguard_cont_install
//    };

//    for (int i = 0; i < containers.size(); ++i) {
//        QPushButton *button = installButtons.at(i);
//        DockerContainer container = containers.at(i);

//        connect(button, &QPushButton::clicked, this, [this, container, button](bool checked){
//            if (checked) {
//                ErrorCode e = doInstallAction([this, container](){
//                    return ServerController::setupContainer(m_settings.serverCredentials(selectedServerIndex), container);
//                },
//                ui->page_server_protocols, ui->progressBar_protocols_container_reinstall,
//                nullptr, nullptr);

//                if (!e) {
//                    m_settings.setContainerConfig(selectedServerIndex, container, QJsonObject());
//                    m_settings.setDefaultContainer(selectedServerIndex, container);
//                }
//            }
//            else {
//                button->setEnabled(false);
//                ErrorCode e = ServerController::removeContainer(m_settings.serverCredentials(selectedServerIndex), container);
//                m_settings.removeContainerConfig(selectedServerIndex, container);
//                button->setEnabled(true);

//                if (m_settings.defaultContainer(selectedServerIndex) == container) {
//                    const auto &c = m_settings.containers(selectedServerIndex);
//                    if (c.isEmpty()) m_settings.setDefaultContainer(selectedServerIndex, DockerContainer::None);
//                    else m_settings.setDefaultContainer(selectedServerIndex, c.keys().first());
//                }
//            }

//            updateProtocolsPage();
//        });
//    }

//    // share buttons
//    QList<QPushButton *> shareButtons {
//        ui->pushButton_proto_openvpn_cont_share,
//                ui->pushButton_proto_ss_openvpn_cont_share,
//                ui->pushButton_proto_cloak_openvpn_cont_share,
//                ui->pushButton_proto_wireguard_cont_share
//    };

//    for (int i = 0; i < containers.size(); ++i) {
//        QPushButton *button = shareButtons.at(i);
//        DockerContainer container = containers.at(i);

//        connect(button, &QPushButton::clicked, this, [this, button, container](){
//            updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), container);
//            goToPage(Page::ShareConnection);
//        });
//    }

//    // settings buttons

//    // settings openvpn container
//    connect(ui->pushButton_proto_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpn;
//        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
//                          selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::OpenVpnSettings);
//    });

//    // settings shadowsocks container
//    connect(ui->pushButton_proto_ss_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
//        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
//                          selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::OpenVpnSettings);
//    });
//    connect(ui->pushButton_proto_ss_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpnOverShadowSocks;
//        updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
//                              selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::ShadowSocksSettings);
//    });

//    // settings cloak container
//    connect(ui->pushButton_proto_cloak_openvpn_cont_openvpn_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
//        updateOpenVpnPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn),
//                          selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::OpenVpnSettings);
//    });
//    connect(ui->pushButton_proto_cloak_openvpn_cont_ss_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
//        updateShadowSocksPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks),
//                              selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::ShadowSocksSettings);
//    });
//    connect(ui->pushButton_proto_cloak_openvpn_cont_cloak_config, &QPushButton::clicked, this, [this](){
//        selectedDockerContainer = DockerContainer::OpenVpnOverCloak;
//        updateCloakPage(m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak),
//                        selectedDockerContainer, m_settings.haveAuthData(selectedServerIndex));
//        goToPage(Page::CloakSettings);
//    });

//    ///
//    // Protocols pages
//    connect(ui->checkBox_proto_openvpn_auto_encryption, &QCheckBox::stateChanged, this, [this](){
//        ui->comboBox_proto_openvpn_cipher->setDisabled(ui->checkBox_proto_openvpn_auto_encryption->isChecked());
//        ui->comboBox_proto_openvpn_hash->setDisabled(ui->checkBox_proto_openvpn_auto_encryption->isChecked());
//    });

//    connect(ui->pushButton_proto_openvpn_save, &QPushButton::clicked, this, [this](){
//        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::OpenVpn);
//        protocolConfig = getOpenVpnConfigFromPage(protocolConfig);

//        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
//        QJsonObject newContainerConfig = containerConfig;
//        newContainerConfig.insert(config_key::openvpn, protocolConfig);

//        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
//            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
//        },
//        ui->page_proto_openvpn, ui->progressBar_proto_openvpn_reset,
//        ui->pushButton_proto_openvpn_save, ui->label_proto_openvpn_info);

//        if (!e) {
//            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
//            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
//        }
//        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
//    });

//    connect(ui->pushButton_proto_shadowsocks_save, &QPushButton::clicked, this, [this](){
//        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::ShadowSocks);
//        protocolConfig = getShadowSocksConfigFromPage(protocolConfig);

//        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
//        QJsonObject newContainerConfig = containerConfig;
//        newContainerConfig.insert(config_key::shadowsocks, protocolConfig);

//        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
//            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
//        },
//        ui->page_proto_shadowsocks, ui->progressBar_proto_shadowsocks_reset,
//        ui->pushButton_proto_shadowsocks_save, ui->label_proto_shadowsocks_info);

//        if (!e) {
//            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
//            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
//        }
//        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
//    });

//    connect(ui->pushButton_proto_cloak_save, &QPushButton::clicked, this, [this](){
//        QJsonObject protocolConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, Protocol::Cloak);
//        protocolConfig = getCloakConfigFromPage(protocolConfig);

//        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
//        QJsonObject newContainerConfig = containerConfig;
//        newContainerConfig.insert(config_key::cloak, protocolConfig);

//        ErrorCode e = doInstallAction([this, containerConfig, newContainerConfig](){
//            return ServerController::updateContainer(m_settings.serverCredentials(selectedServerIndex), selectedDockerContainer, containerConfig, newContainerConfig);
//        },
//        ui->page_proto_cloak, ui->progressBar_proto_cloak_reset,
//        ui->pushButton_proto_cloak_save, ui->label_proto_cloak_info);

//        if (!e) {
//            m_settings.setContainerConfig(selectedServerIndex, selectedDockerContainer, newContainerConfig);
//            m_settings.clearLastConnectionConfig(selectedServerIndex, selectedDockerContainer);
//        }

//        qDebug() << "Protocol saved with code:" << e << "for" << selectedServerIndex << selectedDockerContainer;
//    });
//}

void UiLogic::setupNewServerPageConnections()
{
    //    connect(ui->pushButton_connect, SIGNAL(clicked(bool)), this, SLOT(onPushButtonConnectClicked(bool)));
    //    connect(ui->pushButton_start_switch_page, &QPushButton::toggled, this, [this](bool toggled){
    //        if (toggled){
    //            ui->stackedWidget_start->setCurrentWidget(ui->page_start_new_server);
    //            ui->pushButton_start_switch_page->setText(tr("Import connection"));
    //        }
    //        else {
    //            ui->stackedWidget_start->setCurrentWidget(ui->page_start_import);
    //            ui->pushButton_start_switch_page->setText(tr("Set up your own server"));
    //        }
    //        //goToPage(Page::NewServer);
    //    });

    //    connect(ui->pushButton_new_server_connect_key, &QPushButton::toggled, this, [this](bool checked){
    //        ui->label_new_server_password->setText(checked ? tr("Private key") : tr("Password"));
    //        ui->pushButton_new_server_connect_key->setText(checked ? tr("Connect using SSH password") : tr("Connect using SSH key"));
    //        ui->lineEdit_new_server_password->setVisible(!checked);
    //        ui->textEdit_new_server_ssh_key->setVisible(checked);
    //    });

    //    connect(ui->pushButton_new_server_settings_cloak, &QPushButton::toggled, this, [this](bool toggle){
    //        ui->frame_new_server_settings_cloak->setMaximumHeight(toggle * 200);
    //        if (toggle)
    //            ui->frame_new_server_settings_parent_cloak->layout()->addWidget(ui->frame_new_server_settings_cloak);
    //        else
    //            ui->frame_new_server_settings_parent_cloak->layout()->removeWidget(ui->frame_new_server_settings_cloak);
    //    });
    //    connect(ui->pushButton_new_server_settings_ss, &QPushButton::toggled, this, [this](bool toggle){
    //        ui->frame_new_server_settings_ss->setMaximumHeight(toggle * 200);
    //        if (toggle)
    //            ui->frame_new_server_settings_parent_ss->layout()->addWidget(ui->frame_new_server_settings_ss);
    //        else
    //            ui->frame_new_server_settings_parent_ss->layout()->removeWidget(ui->frame_new_server_settings_ss);
    //    });
    //    connect(ui->pushButton_new_server_settings_openvpn, &QPushButton::toggled, this, [this](bool toggle){
    //        ui->frame_new_server_settings_openvpn->setMaximumHeight(toggle * 200);
    //        if (toggle)
    //            ui->frame_new_server_settings_parent_openvpn->layout()->addWidget(ui->frame_new_server_settings_openvpn);
    //        else
    //            ui->frame_new_server_settings_parent_openvpn->layout()->removeWidget(ui->frame_new_server_settings_ss);
    //    });
}

//void UiLogic::setupServerSettingsPageConnections()
//{
//    connect(ui->pushButton_servers_add_new, &QPushButton::clicked, this, [this](){ goToPage(Page::Start); });

//    connect(ui->pushButton_server_settings_protocols, &QPushButton::clicked, this, [this](){ goToPage(Page::ServerVpnProtocols); });
//    connect(ui->pushButton_server_settings_share_full, &QPushButton::clicked, this, [this](){
//        updateSharingPage(selectedServerIndex, m_settings.serverCredentials(selectedServerIndex), DockerContainer::None);
//        goToPage(Page::ShareConnection);
//    });

//    connect(ui->pushButton_server_settings_clear, SIGNAL(clicked(bool)), this, SLOT(onPushButtonClearServer(bool)));
//    connect(ui->pushButton_server_settings_forget, SIGNAL(clicked(bool)), this, SLOT(onPushButtonForgetServer(bool)));

//    connect(ui->pushButton_server_settings_clear_client_cache, &QPushButton::clicked, this, [this](){
//        ui->pushButton_server_settings_clear_client_cache->setText(tr("Cache cleared"));

//        const auto &containers = m_settings.containers(selectedServerIndex);
//        for (DockerContainer container: containers.keys()) {
//            m_settings.clearLastConnectionConfig(selectedServerIndex, container);
//        }

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_server_settings_clear_client_cache->setText(tr("Clear client cached profile"));
//        });
//    });

//    connect(ui->lineEdit_server_settings_description, &QLineEdit::editingFinished, this, [this](){
//        const QString &newText = ui->lineEdit_server_settings_description->text();
//        QJsonObject server = m_settings.server(selectedServerIndex);
//        server.insert(config_key::description, newText);
//        m_settings.editServer(selectedServerIndex, server);
//        updateServersListPage();
//    });

//    connect(ui->lineEdit_server_settings_description, &QLineEdit::returnPressed, this, [this](){
//        ui->lineEdit_server_settings_description->clearFocus();
//    });
//}

//void UiLogic::setupSharePageConnections()
//{
//    connect(ui->pushButton_share_full_copy, &QPushButton::clicked, this, [this](){
//        QGuiApplication::clipboard()->setText(ui->textEdit_share_full_code->toPlainText());
//        ui->pushButton_share_full_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_full_copy->setText(tr("Copy"));
//        });
//    });

//    connect(ui->pushButton_share_full_save, &QPushButton::clicked, this, [this](){
//        if (ui->textEdit_share_full_code->toPlainText().isEmpty()) return;

//        QString fileName = QFileDialog::getSaveFileName(this, tr("Save AmneziaVPN config"),
//                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
//        QSaveFile save(fileName);
//        save.open(QIODevice::WriteOnly);
//        save.write(ui->textEdit_share_full_code->toPlainText().toUtf8());
//        save.commit();
//    });

//    connect(ui->pushButton_share_amnezia_copy, &QPushButton::clicked, this, [this](){
//        if (ui->textEdit_share_amnezia_code->toPlainText().isEmpty()) return;

//        QGuiApplication::clipboard()->setText(ui->textEdit_share_amnezia_code->toPlainText());
//        ui->pushButton_share_amnezia_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_amnezia_copy->setText(tr("Copy"));
//        });
//    });

//    connect(ui->pushButton_share_amnezia_save, &QPushButton::clicked, this, [this](){
//        if (ui->textEdit_share_amnezia_code->toPlainText().isEmpty()) return;

//        QString fileName = QFileDialog::getSaveFileName(this, tr("Save AmneziaVPN config"),
//                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.amnezia");
//        QSaveFile save(fileName);
//        save.open(QIODevice::WriteOnly);
//        save.write(ui->textEdit_share_amnezia_code->toPlainText().toUtf8());
//        save.commit();
//    });

//    connect(ui->pushButton_share_openvpn_copy, &QPushButton::clicked, this, [this](){
//        QGuiApplication::clipboard()->setText(ui->textEdit_share_openvpn_code->toPlainText());
//        ui->pushButton_share_openvpn_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_openvpn_copy->setText(tr("Copy"));
//        });
//    });

//    connect(ui->pushButton_share_ss_copy, &QPushButton::clicked, this, [this](){
//        QGuiApplication::clipboard()->setText(ui->lineEdit_share_ss_string->text());
//        ui->pushButton_share_ss_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_ss_copy->setText(tr("Copy"));
//        });
//    });

//    connect(ui->pushButton_share_cloak_copy, &QPushButton::clicked, this, [this](){
//        QGuiApplication::clipboard()->setText(ui->plainTextEdit_share_cloak->toPlainText());
//        ui->pushButton_share_cloak_copy->setText(tr("Copied"));

//        QTimer::singleShot(3000, this, [this]() {
//            ui->pushButton_share_cloak_copy->setText(tr("Copy"));
//        });
//    });

//    connect(ui->pushButton_share_amnezia_generate, &QPushButton::clicked, this, [this](){
//        ui->pushButton_share_amnezia_generate->setEnabled(false);
//        ui->pushButton_share_amnezia_copy->setEnabled(false);
//        ui->pushButton_share_amnezia_generate->setText(tr("Generating..."));
//        qApp->processEvents();

//        ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
//        QJsonObject containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);
//        containerConfig.insert(config_key::container, containerToString(selectedDockerContainer));

//        ErrorCode e = ErrorCode::NoError;
//        for (Protocol p: amnezia::protocolsForContainer(selectedDockerContainer)) {
//            QJsonObject protoConfig = m_settings.protocolConfig(selectedServerIndex, selectedDockerContainer, p);

//            QString cfg = VpnConfigurator::genVpnProtocolConfig(credentials, selectedDockerContainer, containerConfig, p, &e);
//            if (e) {
//                cfg = "Error generating config";
//                break;
//            }
//            protoConfig.insert(config_key::last_config, cfg);

//            containerConfig.insert(protoToString(p), protoConfig);
//        }

//        QByteArray ba;
//        if (!e) {
//            QJsonObject serverConfig = m_settings.server(selectedServerIndex);
//            serverConfig.remove(config_key::userName);
//            serverConfig.remove(config_key::password);
//            serverConfig.remove(config_key::port);
//            serverConfig.insert(config_key::containers, QJsonArray {containerConfig});
//            serverConfig.insert(config_key::defaultContainer, containerToString(selectedDockerContainer));


//            ba = QJsonDocument(serverConfig).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
//            ui->textEdit_share_amnezia_code->setPlainText(QString("vpn://%1").arg(QString(ba)));
//        }
//        else {
//            ui->textEdit_share_amnezia_code->setPlainText(tr("Error while generating connection profile"));
//        }

//        ui->pushButton_share_amnezia_generate->setEnabled(true);
//        ui->pushButton_share_amnezia_copy->setEnabled(true);
//        ui->pushButton_share_amnezia_generate->setText(tr("Generate config"));
//    });

//    connect(ui->pushButton_share_openvpn_generate, &QPushButton::clicked, this, [this](){
//        ui->pushButton_share_openvpn_generate->setEnabled(false);
//        ui->pushButton_share_openvpn_copy->setEnabled(false);
//        ui->pushButton_share_openvpn_save->setEnabled(false);
//        ui->pushButton_share_openvpn_generate->setText(tr("Generating..."));

//        ServerCredentials credentials = m_settings.serverCredentials(selectedServerIndex);
//        const QJsonObject &containerConfig = m_settings.containerConfig(selectedServerIndex, selectedDockerContainer);

//        ErrorCode e = ErrorCode::NoError;
//        QString cfg = OpenVpnConfigurator::genOpenVpnConfig(credentials, selectedDockerContainer, containerConfig, &e);
//        cfg = OpenVpnConfigurator::processConfigWithExportSettings(cfg);

//        ui->textEdit_share_openvpn_code->setPlainText(cfg);

//        ui->pushButton_share_openvpn_generate->setEnabled(true);
//        ui->pushButton_share_openvpn_copy->setEnabled(true);
//        ui->pushButton_share_openvpn_save->setEnabled(true);
//        ui->pushButton_share_openvpn_generate->setText(tr("Generate config"));
//    });

//    connect(ui->pushButton_share_openvpn_save, &QPushButton::clicked, this, [this](){
//        QString fileName = QFileDialog::getSaveFileName(this, tr("Save OpenVPN config"),
//                                                        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), "*.ovpn");

//        QSaveFile save(fileName);
//        save.open(QIODevice::WriteOnly);
//        save.write(ui->textEdit_share_openvpn_code->toPlainText().toUtf8());
//        save.commit();
//    });
//}

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
    //    int serverIndex = m_settings.defaultServerIndex();
    //    ServerCredentials credentials = m_settings.serverCredentials(serverIndex);
    //    DockerContainer container = m_settings.defaultContainer(serverIndex);

    //    if (m_settings.containers(serverIndex).isEmpty()) {
    //        ui->label_error_text->setText(tr("VPN Protocols is not installed.\n Please install VPN container at first"));
    //        ui->pushButton_connect->setChecked(false);
    //        return;
    //    }

    //    if (container == DockerContainer::None) {
    //        ui->label_error_text->setText(tr("VPN Protocol not choosen"));
    //        ui->pushButton_connect->setChecked(false);
    //        return;
    //    }


    //    const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);
    //    onConnectWorker(serverIndex, credentials, container, containerConfig);
}

//void UiLogic::onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig)
//{
//    ui->label_error_text->clear();
//    ui->pushButton_connect->setChecked(true);
//    qApp->processEvents();

//    ErrorCode errorCode = m_vpnConnection->connectToVpn(
//                serverIndex, credentials, container, containerConfig
//                );

//    if (errorCode) {
//        //ui->pushButton_connect->setChecked(false);
//        QMessageBox::critical(this, APPLICATION_NAME, errorString(errorCode));
//        return;
//    }

//    ui->pushButton_connect->setEnabled(false);
//}

void UiLogic::onDisconnect()
{
    setPushButtonConnectChecked(false);
    m_vpnConnection->disconnectFromVpn();
}


//void UiLogic::onPushButtonAddCustomSitesClicked()
//{
//    if (ui->radioButton_vpn_mode_all_sites->isChecked()) return;
//    Settings::RouteMode mode = m_settings.routeMode();

//    QString newSite = ui->lineEdit_sites_add_custom->text();

//    if (newSite.isEmpty()) return;
//    if (!newSite.contains(".")) return;

//    if (!Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
//        // get domain name if it present
//        newSite.replace("https://", "");
//        newSite.replace("http://", "");
//        newSite.replace("ftp://", "");

//        newSite = newSite.split("/", QString::SkipEmptyParts).first();
//    }

//    const auto &cbProcess = [this, mode](const QString &newSite, const QString &ip) {
//        m_settings.addVpnSite(mode, newSite, ip);

//        if (!ip.isEmpty()) {
//            m_vpnConnection->addRoutes(QStringList() << ip);
//            m_vpnConnection->flushDns();
//        }
//        else if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
//            m_vpnConnection->addRoutes(QStringList() << newSite);
//            m_vpnConnection->flushDns();
//        }

//        updateSitesPage();
//    };

//    const auto &cbResolv = [this, cbProcess](const QHostInfo &hostInfo){
//        const QList<QHostAddress> &addresses = hostInfo.addresses();
//        QString ipv4Addr;
//        for (const QHostAddress &addr: hostInfo.addresses()) {
//            if (addr.protocol() == QAbstractSocket::NetworkLayerProtocol::IPv4Protocol) {
//                cbProcess(hostInfo.hostName(), addr.toString());
//                break;
//            }
//        }
//    };

//    ui->lineEdit_sites_add_custom->clear();

//    if (Utils::ipAddressWithSubnetRegExp().exactMatch(newSite)) {
//        cbProcess(newSite, "");
//        return;
//    }
//    else {
//        cbProcess(newSite, "");
//        updateSitesPage();
//        QHostInfo::lookupHost(newSite, this, cbResolv);
//    }
//}

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

//void UiLogic::updateSitesPage()
//{
//    Settings::RouteMode m = m_settings.routeMode();
//    if (m == Settings::VpnAllSites) return;

//    if (m == Settings::VpnOnlyForwardSites) ui->label_sites_add_custom->setText(tr("These sites will be opened using VPN"));
//    if (m == Settings::VpnAllExceptSites) ui->label_sites_add_custom->setText(tr("These sites will be excepted from VPN"));

//    ui->tableView_sites->setModel(sitesModels.value(m));
//    sitesModels.value(m)->resetCache();
//}

void UiLogic::updateVpnPage()
{
    Settings::RouteMode mode = m_settings.routeMode();
    setRadioButtonVpnModeAllSitesChecked(mode == Settings::VpnAllSites);
    setRadioButtonVpnModeForwardSitesChecked(mode == Settings::VpnOnlyForwardSites);
    setRadioButtonVpnModeExceptSitesChecked(mode == Settings::VpnAllExceptSites);
    setPushButtonVpnAddSiteEnabled(mode != Settings::VpnAllSites);
}

void UiLogic::updateAppSettingsPage()
{
    setCheckBoxAppSettingsAutostartChecked(Autostart::isAutostart());
    setCheckBoxAppSettingsAutoconnectChecked(m_settings.isAutoConnect());
    setCheckBoxAppSettingsStartMinimizedChecked(m_settings.isStartMinimized());

    setLineEditNetworkSettingsDns1Text(m_settings.primaryDns());
    setLineEditNetworkSettingsDns2Text(m_settings.secondaryDns());

    QString ver = QString("%1: %2 (%3)")
            .arg(tr("Software version"))
            .arg(QString(APP_MAJOR_VERSION))
            .arg(__DATE__);
    setLabelAppSettingsVersionText(ver);
}

void UiLogic::updateGeneralSettingPage()
{
    setPushButtonGeneralSettingsShareConnectionEnable(m_settings.haveAuthData(m_settings.defaultServerIndex()));
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

void UiLogic::onPushButtonAppSettingsOpenLogsChecked()
{
    Debug::openLogsFolder();
}

void UiLogic::onCheckBoxAppSettingsAutostartToggled(bool checked)
{
    if (!checked) {
        setCheckBoxAppSettingsAutoconnectChecked(false);
    }
    Autostart::setAutostart(checked);
}

void UiLogic::onCheckBoxAppSettingsAutoconnectToggled(bool checked)
{
    m_settings.setAutoConnect(checked);
}

void UiLogic::onCheckBoxAppSettingsStartMinimizedToggled(bool checked)
{
    m_settings.setStartMinimized(checked);
}

void UiLogic::onLineEditNetworkSettingsDns1EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setPrimaryDns(text);
    }
}

void UiLogic::onLineEditNetworkSettingsDns2EditFinished(const QString &text)
{
    QRegExp reg{getIpAddressValidatorRegex()};
    if (reg.exactMatch(text)) {
        m_settings.setSecondaryDns(text);
    }
}

void UiLogic::onPushButtonNetworkSettingsResetdns1Clicked()
{
    m_settings.setPrimaryDns(m_settings.cloudFlareNs1);
    updateAppSettingsPage();
}

void UiLogic::onPushButtonNetworkSettingsResetdns2Clicked()
{
    m_settings.setSecondaryDns(m_settings.cloudFlareNs2);
    updateAppSettingsPage();
}

//void UiLogic::updateServersListPage()
//{
//    ui->listWidget_servers->clear();
//    const QJsonArray &servers = m_settings.serversArray();
//    int defaultServer = m_settings.defaultServerIndex();

//    ui->listWidget_servers->setUpdatesEnabled(false);
//    for(int i = 0; i < servers.size(); i++) {
//        makeServersListItem(ui->listWidget_servers, servers.at(i).toObject(), i == defaultServer, i);
//    }
//    ui->listWidget_servers->setUpdatesEnabled(true);
//}

//void UiLogic::updateProtocolsPage()
//{
//    ui->progressBar_protocols_container_reinstall->hide();

//    auto containers = m_settings.containers(selectedServerIndex);
//    DockerContainer defaultContainer = m_settings.defaultContainer(selectedServerIndex);
//    bool haveAuthData = m_settings.haveAuthData(selectedServerIndex);

//    // all containers
//    QList<DockerContainer> allContainers {
//        DockerContainer::OpenVpn,
//                DockerContainer::OpenVpnOverShadowSocks,
//                DockerContainer::OpenVpnOverCloak,
//                DockerContainer::WireGuard
//    };

//    // install buttons
//    QList<QPushButton *> installButtons {
//        ui->pushButton_proto_openvpn_cont_install,
//                ui->pushButton_proto_ss_openvpn_cont_install,
//                ui->pushButton_proto_cloak_openvpn_cont_install,
//                ui->pushButton_proto_wireguard_cont_install
//    };

//    // default buttons
//    QList<QPushButton *> defaultButtons {
//        ui->pushButton_proto_openvpn_cont_default,
//                ui->pushButton_proto_ss_openvpn_cont_default,
//                ui->pushButton_proto_cloak_openvpn_cont_default,
//                ui->pushButton_proto_wireguard_cont_default
//    };

//    // share buttons
//    QList<QPushButton *> shareButtons {
//        ui->pushButton_proto_openvpn_cont_share,
//                ui->pushButton_proto_ss_openvpn_cont_share,
//                ui->pushButton_proto_cloak_openvpn_cont_share,
//                ui->pushButton_proto_wireguard_cont_share
//    };

//    // frames
//    QList<QFrame *> frames {
//        ui->frame_openvpn_settings,
//                ui->frame_openvpn_ss_settings,
//                ui->frame_openvpn_ss_cloak_settings,
//                ui->frame_wireguard_settings
//    };

//    for (int i = 0; i < allContainers.size(); ++i) {
//        defaultButtons.at(i)->setChecked(defaultContainer == allContainers.at(i));
//        defaultButtons.at(i)->setVisible(haveAuthData && containers.contains(allContainers.at(i)));
//        shareButtons.at(i)->setVisible(haveAuthData && containers.contains(allContainers.at(i)));
//        installButtons.at(i)->setChecked(containers.contains(allContainers.at(i)));
//        installButtons.at(i)->setEnabled(haveAuthData);
//        frames.at(i)->setVisible(containers.contains(allContainers.at(i)));

//    }
//}

//void UiLogic::updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData)
//{
//    ui->widget_proto_openvpn->setEnabled(haveAuthData);
//    ui->pushButton_proto_openvpn_save->setVisible(haveAuthData);
//    ui->progressBar_proto_openvpn_reset->setVisible(haveAuthData);

//    ui->radioButton_proto_openvpn_udp->setEnabled(true);
//    ui->radioButton_proto_openvpn_tcp->setEnabled(true);

//    ui->lineEdit_proto_openvpn_subnet->setText(openvpnConfig.value(config_key::subnet_address).
//                                               toString(protocols::openvpn::defaultSubnetAddress));

//    QString trasnsport = openvpnConfig.value(config_key::transport_proto).
//            toString(protocols::openvpn::defaultTransportProto);

//    ui->radioButton_proto_openvpn_udp->setChecked(trasnsport == protocols::openvpn::defaultTransportProto);
//    ui->radioButton_proto_openvpn_tcp->setChecked(trasnsport != protocols::openvpn::defaultTransportProto);

//    ui->comboBox_proto_openvpn_cipher->setCurrentText(openvpnConfig.value(config_key::cipher).
//                                                      toString(protocols::openvpn::defaultCipher));

//    ui->comboBox_proto_openvpn_hash->setCurrentText(openvpnConfig.value(config_key::hash).
//                                                    toString(protocols::openvpn::defaultHash));

//    bool blockOutsideDns = openvpnConfig.value(config_key::block_outside_dns).toBool(protocols::openvpn::defaultBlockOutsideDns);
//    ui->checkBox_proto_openvpn_block_dns->setChecked(blockOutsideDns);

//    bool isNcpDisabled = openvpnConfig.value(config_key::ncp_disable).toBool(protocols::openvpn::defaultNcpDisable);
//    ui->checkBox_proto_openvpn_auto_encryption->setChecked(!isNcpDisabled);

//    bool isTlsAuth = openvpnConfig.value(config_key::tls_auth).toBool(protocols::openvpn::defaultTlsAuth);
//    ui->checkBox_proto_openvpn_tls_auth->setChecked(isTlsAuth);

//    if (container == DockerContainer::OpenVpnOverShadowSocks) {
//        ui->radioButton_proto_openvpn_udp->setEnabled(false);
//        ui->radioButton_proto_openvpn_tcp->setEnabled(false);
//        ui->radioButton_proto_openvpn_tcp->setChecked(true);
//    }

//    ui->lineEdit_proto_openvpn_port->setText(openvpnConfig.value(config_key::port).
//                                             toString(protocols::openvpn::defaultPort));

//    ui->lineEdit_proto_openvpn_port->setEnabled(container == DockerContainer::OpenVpn);
//}

//void UiLogic::updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData)
//{
//    ui->widget_proto_ss->setEnabled(haveAuthData);
//    ui->pushButton_proto_shadowsocks_save->setVisible(haveAuthData);
//    ui->progressBar_proto_shadowsocks_reset->setVisible(haveAuthData);

//    ui->comboBox_proto_shadowsocks_cipher->setCurrentText(ssConfig.value(config_key::cipher).
//                                                          toString(protocols::shadowsocks::defaultCipher));

//    ui->lineEdit_proto_shadowsocks_port->setText(ssConfig.value(config_key::port).
//                                                 toString(protocols::shadowsocks::defaultPort));

//    ui->lineEdit_proto_shadowsocks_port->setEnabled(container == DockerContainer::OpenVpnOverShadowSocks);
//}

//void UiLogic::updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData)
//{
//    ui->widget_proto_cloak->setEnabled(haveAuthData);
//    ui->pushButton_proto_cloak_save->setVisible(haveAuthData);
//    ui->progressBar_proto_cloak_reset->setVisible(haveAuthData);

//    ui->comboBox_proto_cloak_cipher->setCurrentText(ckConfig.value(config_key::cipher).
//                                                    toString(protocols::cloak::defaultCipher));

//    ui->lineEdit_proto_cloak_site->setText(ckConfig.value(config_key::site).
//                                           toString(protocols::cloak::defaultRedirSite));

//    ui->lineEdit_proto_cloak_port->setText(ckConfig.value(config_key::port).
//                                           toString(protocols::cloak::defaultPort));

//    ui->lineEdit_proto_cloak_port->setEnabled(container == DockerContainer::OpenVpnOverCloak);
//}

//void UiLogic::updateSharingPage(int serverIndex, const ServerCredentials &credentials,
//                                DockerContainer container)
//{
//    selectedDockerContainer = container;
//    selectedServerIndex = serverIndex;

//    //const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

//    for (QWidget *page : {
//         ui->page_share_amnezia,
//         ui->page_share_openvpn,
//         ui->page_share_shadowsocks,
//         ui->page_share_cloak,
//         ui->page_share_full_access }) {

//        ui->toolBox_share_connection->removeItem(ui->toolBox_share_connection->indexOf(page));
//        page->hide();
//    }

//    if (container == DockerContainer::OpenVpn) {
//        ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
//        ui->toolBox_share_connection->addItem(ui->page_share_openvpn, tr("  Share for OpenVPN client"));

//        QString cfg = tr("Press Generate config");
//        ui->textEdit_share_openvpn_code->setPlainText(cfg);
//        ui->pushButton_share_openvpn_copy->setEnabled(false);
//        ui->pushButton_share_openvpn_save->setEnabled(false);

//        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_openvpn);
//    }

//    if (container == DockerContainer::OpenVpnOverShadowSocks ||
//            container == DockerContainer::OpenVpnOverCloak) {
//        ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
//        ui->toolBox_share_connection->addItem(ui->page_share_shadowsocks, tr("  Share for ShadowSocks client"));

//        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::ShadowSocks);
//        QString cfg = protoConfig.value(config_key::last_config).toString();

//        if (cfg.isEmpty()) {
//            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

//            ErrorCode e = ErrorCode::NoError;
//            cfg = ShadowSocksConfigurator::genShadowSocksConfig(credentials, container, containerConfig, &e);

//            ui->pushButton_share_ss_copy->setEnabled(true);
//        }

//        QJsonObject ssConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();

//        QString ssString = QString("%1:%2@%3:%4")
//                .arg(ssConfig.value("method").toString())
//                .arg(ssConfig.value("password").toString())
//                .arg(ssConfig.value("server").toString())
//                .arg(ssConfig.value("server_port").toString());

//        ssString = "ss://" + ssString.toUtf8().toBase64();
//        ui->lineEdit_share_ss_string->setText(ssString);
//        updateQRCodeImage(ssString, ui->label_share_ss_qr_code);

//        ui->label_share_ss_server->setText(ssConfig.value("server").toString());
//        ui->label_share_ss_port->setText(ssConfig.value("server_port").toString());
//        ui->label_share_ss_method->setText(ssConfig.value("method").toString());
//        ui->label_share_ss_password->setText(ssConfig.value("password").toString());

//        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_shadowsocks);
//        ui->page_share_shadowsocks->show();
//        ui->page_share_shadowsocks->raise();
//        qDebug() << ui->page_share_shadowsocks->size();
//        ui->toolBox_share_connection->layout()->update();
//    }

//    if (container == DockerContainer::OpenVpnOverCloak) {
//        //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));
//        ui->toolBox_share_connection->addItem(ui->page_share_cloak, tr("  Share for Cloak client"));
//        ui->plainTextEdit_share_cloak->setPlainText(QString(""));

//        QJsonObject protoConfig = m_settings.protocolConfig(serverIndex, container, Protocol::Cloak);
//        QString cfg = protoConfig.value(config_key::last_config).toString();

//        if (cfg.isEmpty()) {
//            const QJsonObject &containerConfig = m_settings.containerConfig(serverIndex, container);

//            ErrorCode e = ErrorCode::NoError;
//            cfg = CloakConfigurator::genCloakConfig(credentials, container, containerConfig, &e);

//            ui->pushButton_share_cloak_copy->setEnabled(true);
//        }

//        QJsonObject cloakConfig = QJsonDocument::fromJson(cfg.toUtf8()).object();
//        cloakConfig.remove(config_key::transport_proto);
//        cloakConfig.insert("ProxyMethod", "shadowsocks");

//        ui->plainTextEdit_share_cloak->setPlainText(QJsonDocument(cloakConfig).toJson());
//    }

//    // Full access
//    if (container == DockerContainer::None) {
//        ui->toolBox_share_connection->addItem(ui->page_share_full_access, tr("  Share server full access"));

//        const QJsonObject &server = m_settings.server(selectedServerIndex);

//        QByteArray ba = QJsonDocument(server).toJson().toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

//        ui->textEdit_share_full_code->setText(QString("vpn://%1").arg(QString(ba)));
//        ui->toolBox_share_connection->setCurrentWidget(ui->page_share_full_access);
//    }

//    //ui->toolBox_share_connection->addItem(ui->page_share_amnezia, tr("  Share for Amnezia client"));

//    // Amnezia sharing
//    //    QJsonObject exportContainer;
//    //    for (Protocol p: protocolsForContainer(container)) {
//    //        QJsonObject protocolConfig = containerConfig.value(protoToString(p)).toObject();
//    //        protocolConfig.remove(config_key::last_config);
//    //        exportContainer.insert(protoToString(p), protocolConfig);
//    //    }
//    //    exportContainer.insert(config_key::container, containerToString(container));

//    //    ui->textEdit_share_amnezia_code->setPlainText(QJsonDocument(exportContainer).toJson());

//    ui->textEdit_share_amnezia_code->setPlainText(tr(""));
//}

//void UiLogic::makeServersListItem(QListWidget *listWidget, const QJsonObject &server, bool isDefault, int index)
//{
//    QSize size(310, 70);
//    ServerWidget* widget = new ServerWidget(server, isDefault);

//    widget->resize(size);

//    connect(widget->ui->pushButton_default, &QPushButton::clicked, this, [this, index](){
//        m_settings.setDefaultServer(index);
//        updateServersListPage();
//    });

//    //    connect(widget->ui->pushButton_share, &QPushButton::clicked, this, [this, index](){
//    //        goToPage(Page::ShareConnection);
//    //        // update share page
//    //    });

//    connect(widget->ui->pushButton_settings, &QPushButton::clicked, this, [this, index](){
//        selectedServerIndex = index;
//        goToPage(Page::ServerSettings);
//    });

//    QListWidgetItem* item = new QListWidgetItem(listWidget);
//    item->setSizeHint(size);
//    listWidget->setItemWidget(item, widget);

//    widget->setStyleSheet(styleSheet());
//}

//void UiLogic::updateQRCodeImage(const QString &text, QLabel *label)
//{
//    int levelIndex = 1;
//    int versionIndex = 0;
//    bool bExtent = true;
//    int maskIndex = -1;

//    m_qrEncode.EncodeData( levelIndex, versionIndex, bExtent, maskIndex, text.toUtf8().data() );

//    int qrImageSize = m_qrEncode.m_nSymbleSize;

//    int encodeImageSize = qrImageSize + ( QR_MARGIN * 2 );
//    QImage encodeImage( encodeImageSize, encodeImageSize, QImage::Format_Mono );

//    encodeImage.fill( 1 );

//    for ( int i = 0; i < qrImageSize; i++ )
//        for ( int j = 0; j < qrImageSize; j++ )
//            if ( m_qrEncode.m_byModuleData[i][j] )
//                encodeImage.setPixel( i + QR_MARGIN, j + QR_MARGIN, 0 );

//    label->setPixmap(QPixmap::fromImage(encodeImage.scaledToWidth(label->width())));
//}

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
