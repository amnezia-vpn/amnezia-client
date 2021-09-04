#ifndef UILOGIC_H
#define UILOGIC_H

#include <QRegExpValidator>
#include <QQmlEngine>
#include <functional>

#include "pages.h"
#include "protocols/vpnprotocol.h"

#include "settings.h"

class AppSettingsLogic;
class GeneralSettingsLogic;
class NetworkSettingsLogic;
class NewServerLogic;
class ProtocolSettingsLogic;
class ServerListLogic;
class ServerSettingsLogic;
class ServerVpnProtocolsLogic;
class ShareConnectionLogic;
class SitesLogic;
class StartPageLogic;
class VpnLogic;
class WizardLogic;

class VpnConnection;



class UiLogic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool frameWireguardSettingsVisible READ getFrameWireguardSettingsVisible WRITE setFrameWireguardSettingsVisible NOTIFY frameWireguardSettingsVisibleChanged)
    Q_PROPERTY(bool frameWireguardVisible READ getFrameWireguardVisible WRITE setFrameWireguardVisible NOTIFY frameWireguardVisibleChanged)
    Q_PROPERTY(bool frameNewServerSettingsParentWireguardVisible READ getFrameNewServerSettingsParentWireguardVisible WRITE setFrameNewServerSettingsParentWireguardVisible NOTIFY frameNewServerSettingsParentWireguardVisibleChanged)
    Q_PROPERTY(bool radioButtonSetupWizardMediumChecked READ getRadioButtonSetupWizardMediumChecked WRITE setRadioButtonSetupWizardMediumChecked NOTIFY radioButtonSetupWizardMediumCheckedChanged)
    Q_PROPERTY(QString lineEditSetupWizardHighWebsiteMaskingText READ getLineEditSetupWizardHighWebsiteMaskingText WRITE setLineEditSetupWizardHighWebsiteMaskingText NOTIFY lineEditSetupWizardHighWebsiteMaskingTextChanged)
    Q_PROPERTY(double progressBarNewServerConfiguringValue READ getProgressBarNewServerConfiguringValue WRITE setProgressBarNewServerConfiguringValue NOTIFY progressBarNewServerConfiguringValueChanged)
    Q_PROPERTY(bool pushButtonNewServerSettingsCloakChecked READ getPushButtonNewServerSettingsCloakChecked WRITE setPushButtonNewServerSettingsCloakChecked NOTIFY pushButtonNewServerSettingsCloakCheckedChanged)
    Q_PROPERTY(bool pushButtonNewServerSettingsSsChecked READ getPushButtonNewServerSettingsSsChecked WRITE setPushButtonNewServerSettingsSsChecked NOTIFY pushButtonNewServerSettingsSsCheckedChanged)
    Q_PROPERTY(bool pushButtonNewServerSettingsOpenvpnChecked READ getPushButtonNewServerSettingsOpenvpnChecked WRITE setPushButtonNewServerSettingsOpenvpnChecked NOTIFY pushButtonNewServerSettingsOpenvpnCheckedChanged)
    Q_PROPERTY(QString lineEditNewServerCloakPortText READ getLineEditNewServerCloakPortText WRITE setLineEditNewServerCloakPortText NOTIFY lineEditNewServerCloakPortTextChanged)
    Q_PROPERTY(QString lineEditNewServerCloakSiteText READ getLineEditNewServerCloakSiteText WRITE setLineEditNewServerCloakSiteText NOTIFY lineEditNewServerCloakSiteTextChanged)
    Q_PROPERTY(QString lineEditNewServerSsPortText READ getLineEditNewServerSsPortText WRITE setLineEditNewServerSsPortText NOTIFY lineEditNewServerSsPortTextChanged)
    Q_PROPERTY(QString comboBoxNewServerSsCipherText READ getComboBoxNewServerSsCipherText WRITE setComboBoxNewServerSsCipherText NOTIFY comboBoxNewServerSsCipherTextChanged)
    Q_PROPERTY(QString lineEditNewServerOpenvpnPortText READ getlineEditNewServerOpenvpnPortText WRITE setLineEditNewServerOpenvpnPortText NOTIFY lineEditNewServerOpenvpnPortTextChanged)
    Q_PROPERTY(QString comboBoxNewServerOpenvpnProtoText READ getComboBoxNewServerOpenvpnProtoText WRITE setComboBoxNewServerOpenvpnProtoText NOTIFY comboBoxNewServerOpenvpnProtoTextChanged)


    Q_PROPERTY(int currentPageValue READ getCurrentPageValue WRITE setCurrentPageValue NOTIFY currentPageValueChanged)
    Q_PROPERTY(QString trayIconUrl READ getTrayIconUrl WRITE setTrayIconUrl NOTIFY trayIconUrlChanged)
    Q_PROPERTY(bool trayActionDisconnectEnabled READ getTrayActionDisconnectEnabled WRITE setTrayActionDisconnectEnabled NOTIFY trayActionDisconnectEnabledChanged)
    Q_PROPERTY(bool trayActionConnectEnabled READ getTrayActionConnectEnabled WRITE setTrayActionConnectEnabled NOTIFY trayActionConnectEnabledChanged)
    Q_PROPERTY(bool checkBoxNewServerCloakChecked READ getCheckBoxNewServerCloakChecked WRITE setCheckBoxNewServerCloakChecked NOTIFY checkBoxNewServerCloakCheckedChanged)
    Q_PROPERTY(bool checkBoxNewServerSsChecked READ getCheckBoxNewServerSsChecked WRITE setCheckBoxNewServerSsChecked NOTIFY checkBoxNewServerSsCheckedChanged)
    Q_PROPERTY(bool checkBoxNewServerOpenvpnChecked READ getCheckBoxNewServerOpenvpnChecked WRITE setCheckBoxNewServerOpenvpnChecked NOTIFY checkBoxNewServerOpenvpnCheckedChanged)
    Q_PROPERTY(QString comboBoxProtoCloakCipherText READ getComboBoxProtoCloakCipherText WRITE setComboBoxProtoCloakCipherText NOTIFY comboBoxProtoCloakCipherTextChanged)
    Q_PROPERTY(QString lineEditProtoCloakSiteText READ getLineEditProtoCloakSiteText WRITE setLineEditProtoCloakSiteText NOTIFY lineEditProtoCloakSiteTextChanged)
    Q_PROPERTY(QString lineEditProtoCloakPortText READ getLineEditProtoCloakPortText WRITE setLineEditProtoCloakPortText NOTIFY lineEditProtoCloakPortTextChanged)
    Q_PROPERTY(QString comboBoxProtoShadowsocksCipherText READ getComboBoxProtoShadowsocksCipherText WRITE setComboBoxProtoShadowsocksCipherText NOTIFY comboBoxProtoShadowsocksCipherTextChanged)
    Q_PROPERTY(QString lineEditProtoShadowsocksPortText READ getLineEditProtoShadowsocksPortText WRITE setLineEditProtoShadowsocksPortText NOTIFY lineEditProtoShadowsocksPortTextChanged)
    Q_PROPERTY(QString lineEditProtoOpenvpnSubnetText READ getLineEditProtoOpenvpnSubnetText WRITE setLineEditProtoOpenvpnSubnetText NOTIFY lineEditProtoOpenvpnSubnetTextChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnUdpChecked READ getRadioButtonProtoOpenvpnUdpChecked WRITE setRadioButtonProtoOpenvpnUdpChecked NOTIFY radioButtonProtoOpenvpnUdpCheckedChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnAutoEncryptionChecked READ getCheckBoxProtoOpenvpnAutoEncryptionChecked WRITE setCheckBoxProtoOpenvpnAutoEncryptionChecked NOTIFY checkBoxProtoOpenvpnAutoEncryptionCheckedChanged)
    Q_PROPERTY(QString comboBoxProtoOpenvpnCipherText READ getComboBoxProtoOpenvpnCipherText WRITE setComboBoxProtoOpenvpnCipherText NOTIFY comboBoxProtoOpenvpnCipherTextChanged)
    Q_PROPERTY(QString comboBoxProtoOpenvpnHashText READ getComboBoxProtoOpenvpnHashText WRITE setComboBoxProtoOpenvpnHashText NOTIFY comboBoxProtoOpenvpnHashTextChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnBlockDnsChecked READ getCheckBoxProtoOpenvpnBlockDnsChecked WRITE setCheckBoxProtoOpenvpnBlockDnsChecked NOTIFY checkBoxProtoOpenvpnBlockDnsCheckedChanged)
    Q_PROPERTY(QString lineEditProtoOpenvpnPortText READ getLineEditProtoOpenvpnPortText WRITE setLineEditProtoOpenvpnPortText NOTIFY lineEditProtoOpenvpnPortTextChanged)
    Q_PROPERTY(bool checkBoxProtoOpenvpnTlsAuthChecked READ getCheckBoxProtoOpenvpnTlsAuthChecked WRITE setCheckBoxProtoOpenvpnTlsAuthChecked NOTIFY checkBoxProtoOpenvpnTlsAuthCheckedChanged)
    Q_PROPERTY(bool radioButtonSetupWizardHighChecked READ getRadioButtonSetupWizardHighChecked WRITE setRadioButtonSetupWizardHighChecked NOTIFY radioButtonSetupWizardHighCheckedChanged)
    Q_PROPERTY(bool radioButtonSetupWizardLowChecked READ getRadioButtonSetupWizardLowChecked WRITE setRadioButtonSetupWizardLowChecked NOTIFY radioButtonSetupWizardLowCheckedChanged)
    Q_PROPERTY(bool checkBoxSetupWizardVpnModeChecked READ getCheckBoxSetupWizardVpnModeChecked WRITE setCheckBoxSetupWizardVpnModeChecked NOTIFY checkBoxSetupWizardVpnModeCheckedChanged)

    Q_PROPERTY(bool pushButtonConnectChecked READ getPushButtonConnectChecked WRITE setPushButtonConnectChecked NOTIFY pushButtonConnectCheckedChanged)
    Q_PROPERTY(bool widgetProtoCloakEnabled READ getWidgetProtoCloakEnabled WRITE setWidgetProtoCloakEnabled NOTIFY widgetProtoCloakEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoCloakSaveVisible READ getPushButtonProtoCloakSaveVisible WRITE setPushButtonProtoCloakSaveVisible NOTIFY pushButtonProtoCloakSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoCloakResetVisible READ getProgressBarProtoCloakResetVisible WRITE setProgressBarProtoCloakResetVisible NOTIFY progressBarProtoCloakResetVisibleChanged)
    Q_PROPERTY(bool lineEditProtoCloakPortEnabled READ getLineEditProtoCloakPortEnabled WRITE setLineEditProtoCloakPortEnabled NOTIFY lineEditProtoCloakPortEnabledChanged)
    Q_PROPERTY(bool widgetProtoSsEnabled READ getWidgetProtoSsEnabled WRITE setWidgetProtoSsEnabled NOTIFY widgetProtoSsEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoShadowsocksSaveVisible READ getPushButtonProtoShadowsocksSaveVisible WRITE setPushButtonProtoShadowsocksSaveVisible NOTIFY pushButtonProtoShadowsocksSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoShadowsocksResetVisible READ getProgressBarProtoShadowsocksResetVisible WRITE setProgressBarProtoShadowsocksResetVisible NOTIFY progressBarProtoShadowsocksResetVisibleChanged)
    Q_PROPERTY(bool lineEditProtoShadowsocksPortEnabled READ getLineEditProtoShadowsocksPortEnabled WRITE setLineEditProtoShadowsocksPortEnabled NOTIFY lineEditProtoShadowsocksPortEnabledChanged)
    Q_PROPERTY(bool widgetProtoOpenvpnEnabled READ getWidgetProtoOpenvpnEnabled WRITE setWidgetProtoOpenvpnEnabled NOTIFY widgetProtoOpenvpnEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnSaveVisible READ getPushButtonProtoOpenvpnSaveVisible WRITE setPushButtonProtoOpenvpnSaveVisible NOTIFY pushButtonProtoOpenvpnSaveVisibleChanged)
    Q_PROPERTY(bool progressBarProtoOpenvpnResetVisible READ getProgressBarProtoOpenvpnResetVisible WRITE setProgressBarProtoOpenvpnResetVisible NOTIFY progressBarProtoOpenvpnResetVisibleChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnUdpEnabled READ getRadioButtonProtoOpenvpnUdpEnabled WRITE setRadioButtonProtoOpenvpnUdpEnabled NOTIFY radioButtonProtoOpenvpnUdpEnabledChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnTcpEnabled READ getRadioButtonProtoOpenvpnTcpEnabled WRITE setRadioButtonProtoOpenvpnTcpEnabled NOTIFY radioButtonProtoOpenvpnTcpEnabledChanged)
    Q_PROPERTY(bool radioButtonProtoOpenvpnTcpChecked READ getRadioButtonProtoOpenvpnTcpChecked WRITE setRadioButtonProtoOpenvpnTcpChecked NOTIFY radioButtonProtoOpenvpnTcpCheckedChanged)
    Q_PROPERTY(bool lineEditProtoOpenvpnPortEnabled READ getLineEditProtoOpenvpnPortEnabled WRITE setLineEditProtoOpenvpnPortEnabled NOTIFY lineEditProtoOpenvpnPortEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContInstallChecked READ getPushButtonProtoOpenvpnContInstallChecked WRITE setPushButtonProtoOpenvpnContInstallChecked NOTIFY pushButtonProtoOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContInstallChecked READ getPushButtonProtoSsOpenvpnContInstallChecked WRITE setPushButtonProtoSsOpenvpnContInstallChecked NOTIFY pushButtonProtoSsOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContInstallChecked READ getPushButtonProtoCloakOpenvpnContInstallChecked WRITE setPushButtonProtoCloakOpenvpnContInstallChecked NOTIFY pushButtonProtoCloakOpenvpnContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContInstallChecked READ getPushButtonProtoWireguardContInstallChecked WRITE setPushButtonProtoWireguardContInstallChecked NOTIFY pushButtonProtoWireguardContInstallCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContInstallEnabled READ getPushButtonProtoOpenvpnContInstallEnabled WRITE setPushButtonProtoOpenvpnContInstallEnabled NOTIFY pushButtonProtoOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContInstallEnabled READ getPushButtonProtoSsOpenvpnContInstallEnabled WRITE setPushButtonProtoSsOpenvpnContInstallEnabled NOTIFY pushButtonProtoSsOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContInstallEnabled READ getPushButtonProtoCloakOpenvpnContInstallEnabled WRITE setPushButtonProtoCloakOpenvpnContInstallEnabled NOTIFY pushButtonProtoCloakOpenvpnContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContInstallEnabled READ getPushButtonProtoWireguardContInstallEnabled WRITE setPushButtonProtoWireguardContInstallEnabled NOTIFY pushButtonProtoWireguardContInstallEnabledChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContDefaultChecked READ getPushButtonProtoOpenvpnContDefaultChecked WRITE setPushButtonProtoOpenvpnContDefaultChecked NOTIFY pushButtonProtoOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContDefaultChecked READ getPushButtonProtoSsOpenvpnContDefaultChecked WRITE setPushButtonProtoSsOpenvpnContDefaultChecked NOTIFY pushButtonProtoSsOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContDefaultChecked READ getPushButtonProtoCloakOpenvpnContDefaultChecked WRITE setPushButtonProtoCloakOpenvpnContDefaultChecked NOTIFY pushButtonProtoCloakOpenvpnContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContDefaultChecked READ getPushButtonProtoWireguardContDefaultChecked WRITE setPushButtonProtoWireguardContDefaultChecked NOTIFY pushButtonProtoWireguardContDefaultCheckedChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContDefaultVisible READ getPushButtonProtoOpenvpnContDefaultVisible WRITE setPushButtonProtoOpenvpnContDefaultVisible NOTIFY pushButtonProtoOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContDefaultVisible READ getPushButtonProtoSsOpenvpnContDefaultVisible WRITE setPushButtonProtoSsOpenvpnContDefaultVisible NOTIFY pushButtonProtoSsOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContDefaultVisible READ getPushButtonProtoCloakOpenvpnContDefaultVisible WRITE setPushButtonProtoCloakOpenvpnContDefaultVisible NOTIFY pushButtonProtoCloakOpenvpnContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContDefaultVisible READ getPushButtonProtoWireguardContDefaultVisible WRITE setPushButtonProtoWireguardContDefaultVisible NOTIFY pushButtonProtoWireguardContDefaultVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoOpenvpnContShareVisible READ getPushButtonProtoOpenvpnContShareVisible WRITE setPushButtonProtoOpenvpnContShareVisible NOTIFY pushButtonProtoOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoSsOpenvpnContShareVisible READ getPushButtonProtoSsOpenvpnContShareVisible WRITE setPushButtonProtoSsOpenvpnContShareVisible NOTIFY pushButtonProtoSsOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoCloakOpenvpnContShareVisible READ getPushButtonProtoCloakOpenvpnContShareVisible WRITE setPushButtonProtoCloakOpenvpnContShareVisible NOTIFY pushButtonProtoCloakOpenvpnContShareVisibleChanged)
    Q_PROPERTY(bool pushButtonProtoWireguardContShareVisible READ getPushButtonProtoWireguardContShareVisible WRITE setPushButtonProtoWireguardContShareVisible NOTIFY pushButtonProtoWireguardContShareVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSettingsVisible READ getFrameOpenvpnSettingsVisible WRITE setFrameOpenvpnSettingsVisible NOTIFY frameOpenvpnSettingsVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSsSettingsVisible READ getFrameOpenvpnSsSettingsVisible WRITE setFrameOpenvpnSsSettingsVisible NOTIFY frameOpenvpnSsSettingsVisibleChanged)
    Q_PROPERTY(bool frameOpenvpnSsCloakSettingsVisible READ getFrameOpenvpnSsCloakSettingsVisible WRITE setFrameOpenvpnSsCloakSettingsVisible NOTIFY frameOpenvpnSsCloakSettingsVisibleChanged)
    Q_PROPERTY(bool progressBarProtocolsContainerReinstallVisible READ getProgressBarProtocolsContainerReinstallVisible WRITE setProgressBarProtocolsContainerReinstallVisible NOTIFY progressBarProtocolsContainerReinstallVisibleChanged)
    Q_PROPERTY(QString labelSpeedReceivedText READ getLabelSpeedReceivedText WRITE setLabelSpeedReceivedText NOTIFY labelSpeedReceivedTextChanged)
    Q_PROPERTY(QString labelSpeedSentText READ getLabelSpeedSentText WRITE setLabelSpeedSentText NOTIFY labelSpeedSentTextChanged)
    Q_PROPERTY(QString labelStateText READ getLabelStateText WRITE setLabelStateText NOTIFY labelStateTextChanged)
    Q_PROPERTY(bool pushButtonConnectEnabled READ getPushButtonConnectEnabled WRITE setPushButtonConnectEnabled NOTIFY pushButtonConnectEnabledChanged)
    Q_PROPERTY(bool widgetVpnModeEnabled READ getWidgetVpnModeEnabled WRITE setWidgetVpnModeEnabled NOTIFY widgetVpnModeEnabledChanged)
    Q_PROPERTY(QString labelErrorText READ getLabelErrorText WRITE setLabelErrorText NOTIFY labelErrorTextChanged)
    Q_PROPERTY(QString dialogConnectErrorText READ getDialogConnectErrorText WRITE setDialogConnectErrorText NOTIFY dialogConnectErrorTextChanged)
    Q_PROPERTY(bool pageNewServerConfiguringEnabled READ getPageNewServerConfiguringEnabled WRITE setPageNewServerConfiguringEnabled NOTIFY pageNewServerConfiguringEnabledChanged)
    Q_PROPERTY(bool labelNewServerConfiguringWaitInfoVisible READ getLabelNewServerConfiguringWaitInfoVisible WRITE setLabelNewServerConfiguringWaitInfoVisible NOTIFY labelNewServerConfiguringWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelNewServerConfiguringWaitInfoText READ getLabelNewServerConfiguringWaitInfoText WRITE setLabelNewServerConfiguringWaitInfoText NOTIFY labelNewServerConfiguringWaitInfoTextChanged)
    Q_PROPERTY(bool progressBarNewServerConfiguringVisible READ getProgressBarNewServerConfiguringVisible WRITE setProgressBarNewServerConfiguringVisible NOTIFY progressBarNewServerConfiguringVisibleChanged)
    Q_PROPERTY(int progressBarNewServerConfiguringMaximium READ getProgressBarNewServerConfiguringMaximium WRITE setProgressBarNewServerConfiguringMaximium NOTIFY progressBarNewServerConfiguringMaximiumChanged)
    Q_PROPERTY(bool progressBarNewServerConfiguringTextVisible READ getProgressBarNewServerConfiguringTextVisible WRITE setProgressBarNewServerConfiguringTextVisible NOTIFY progressBarNewServerConfiguringTextVisibleChanged)
    Q_PROPERTY(QString progressBarNewServerConfiguringText READ getProgressBarNewServerConfiguringText WRITE setProgressBarNewServerConfiguringText NOTIFY progressBarNewServerConfiguringTextChanged)
    Q_PROPERTY(bool pageServerProtocolsEnabled READ getPageServerProtocolsEnabled WRITE setPageServerProtocolsEnabled NOTIFY pageServerProtocolsEnabledChanged)
    Q_PROPERTY(int progressBarProtocolsContainerReinstallValue READ getProgressBarProtocolsContainerReinstallValue WRITE setProgressBarProtocolsContainerReinstallValue NOTIFY progressBarProtocolsContainerReinstallValueChanged)
    Q_PROPERTY(int progressBarProtocolsContainerReinstallMaximium READ getProgressBarProtocolsContainerReinstallMaximium WRITE setProgressBarProtocolsContainerReinstallMaximium NOTIFY progressBarProtocolsContainerReinstallMaximiumChanged)
    Q_PROPERTY(bool comboBoxProtoOpenvpnCipherEnabled READ getComboBoxProtoOpenvpnCipherEnabled WRITE setComboBoxProtoOpenvpnCipherEnabled NOTIFY comboBoxProtoOpenvpnCipherEnabledChanged)
    Q_PROPERTY(bool comboBoxProtoOpenvpnHashEnabled READ getComboBoxProtoOpenvpnHashEnabled WRITE setComboBoxProtoOpenvpnHashEnabled NOTIFY comboBoxProtoOpenvpnHashEnabledChanged)
    Q_PROPERTY(bool pageProtoOpenvpnEnabled READ getPageProtoOpenvpnEnabled WRITE setPageProtoOpenvpnEnabled NOTIFY pageProtoOpenvpnEnabledChanged)
    Q_PROPERTY(bool labelProtoOpenvpnInfoVisible READ getLabelProtoOpenvpnInfoVisible WRITE setLabelProtoOpenvpnInfoVisible NOTIFY labelProtoOpenvpnInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoOpenvpnInfoText READ getLabelProtoOpenvpnInfoText WRITE setLabelProtoOpenvpnInfoText NOTIFY labelProtoOpenvpnInfoTextChanged)
    Q_PROPERTY(int progressBarProtoOpenvpnResetValue READ getProgressBarProtoOpenvpnResetValue WRITE setProgressBarProtoOpenvpnResetValue NOTIFY progressBarProtoOpenvpnResetValueChanged)
    Q_PROPERTY(int progressBarProtoOpenvpnResetMaximium READ getProgressBarProtoOpenvpnResetMaximium WRITE setProgressBarProtoOpenvpnResetMaximium NOTIFY progressBarProtoOpenvpnResetMaximiumChanged)
    Q_PROPERTY(bool pageProtoShadowsocksEnabled READ getPageProtoShadowsocksEnabled WRITE setPageProtoShadowsocksEnabled NOTIFY pageProtoShadowsocksEnabledChanged)
    Q_PROPERTY(bool labelProtoShadowsocksInfoVisible READ getLabelProtoShadowsocksInfoVisible WRITE setLabelProtoShadowsocksInfoVisible NOTIFY labelProtoShadowsocksInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoShadowsocksInfoText READ getLabelProtoShadowsocksInfoText WRITE setLabelProtoShadowsocksInfoText NOTIFY labelProtoShadowsocksInfoTextChanged)
    Q_PROPERTY(int progressBarProtoShadowsocksResetValue READ getProgressBarProtoShadowsocksResetValue WRITE setProgressBarProtoShadowsocksResetValue NOTIFY progressBarProtoShadowsocksResetValueChanged)
    Q_PROPERTY(int progressBarProtoShadowsocksResetMaximium READ getProgressBarProtoShadowsocksResetMaximium WRITE setProgressBarProtoShadowsocksResetMaximium NOTIFY progressBarProtoShadowsocksResetMaximiumChanged)
    Q_PROPERTY(bool pageProtoCloakEnabled READ getPageProtoCloakEnabled WRITE setPageProtoCloakEnabled NOTIFY pageProtoCloakEnabledChanged)
    Q_PROPERTY(bool labelProtoCloakInfoVisible READ getLabelProtoCloakInfoVisible WRITE setLabelProtoCloakInfoVisible NOTIFY labelProtoCloakInfoVisibleChanged)
    Q_PROPERTY(QString labelProtoCloakInfoText READ getLabelProtoCloakInfoText WRITE setLabelProtoCloakInfoText NOTIFY labelProtoCloakInfoTextChanged)
    Q_PROPERTY(int progressBarProtoCloakResetValue READ getProgressBarProtoCloakResetValue WRITE setProgressBarProtoCloakResetValue NOTIFY progressBarProtoCloakResetValueChanged)
    Q_PROPERTY(int progressBarProtoCloakResetMaximium READ getProgressBarProtoCloakResetMaximium WRITE setProgressBarProtoCloakResetMaximium NOTIFY progressBarProtoCloakResetMaximiumChanged)


    Q_PROPERTY(bool pushButtonVpnAddSiteEnabled READ getPushButtonVpnAddSiteEnabled WRITE setPushButtonVpnAddSiteEnabled NOTIFY pushButtonVpnAddSiteEnabledChanged)

    Q_PROPERTY(bool radioButtonVpnModeAllSitesChecked READ getRadioButtonVpnModeAllSitesChecked WRITE setRadioButtonVpnModeAllSitesChecked NOTIFY radioButtonVpnModeAllSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeForwardSitesChecked READ getRadioButtonVpnModeForwardSitesChecked WRITE setRadioButtonVpnModeForwardSitesChecked NOTIFY radioButtonVpnModeForwardSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeExceptSitesChecked READ getRadioButtonVpnModeExceptSitesChecked WRITE setRadioButtonVpnModeExceptSitesChecked NOTIFY radioButtonVpnModeExceptSitesCheckedChanged)

public:
    explicit UiLogic(QObject *parent = nullptr);
    ~UiLogic();
    void showOnStartup();

    friend class AppSettingsLogic;
    friend class GeneralSettingsLogic;
    friend class NetworkSettingsLogic;
    friend class NewServerLogic;
    friend class ProtocolSettingsLogic;
    friend class ServerListLogic;
    friend class ServerSettingsLogic;
    friend class ServerVpnProtocolsLogic;
    friend class ShareConnectionLogic;
    friend class SitesLogic;
    friend class StartPageLogic;
    friend class VpnLogic;
    friend class WizardLogic;

    Q_INVOKABLE void initalizeUiLogic();


    bool getFrameWireguardSettingsVisible() const;
    void setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible);
    bool getFrameWireguardVisible() const;
    void setFrameWireguardVisible(bool frameWireguardVisible);
    bool getFrameNewServerSettingsParentWireguardVisible() const;
    void setFrameNewServerSettingsParentWireguardVisible(bool frameNewServerSettingsParentWireguardVisible);
    bool getRadioButtonSetupWizardMediumChecked() const;
    void setRadioButtonSetupWizardMediumChecked(bool radioButtonSetupWizardMediumChecked);
    QString getLineEditSetupWizardHighWebsiteMaskingText() const;
    void setLineEditSetupWizardHighWebsiteMaskingText(const QString &lineEditSetupWizardHighWebsiteMaskingText);
    double getProgressBarNewServerConfiguringValue() const;
    void setProgressBarNewServerConfiguringValue(double progressBarNewServerConfiguringValue);
    bool getPushButtonNewServerSettingsCloakChecked() const;
    void setPushButtonNewServerSettingsCloakChecked(bool pushButtonNewServerSettingsCloakChecked);
    bool getPushButtonNewServerSettingsSsChecked() const;
    void setPushButtonNewServerSettingsSsChecked(bool pushButtonNewServerSettingsSsChecked);
    bool getPushButtonNewServerSettingsOpenvpnChecked() const;
    void setPushButtonNewServerSettingsOpenvpnChecked(bool pushButtonNewServerSettingsOpenvpnChecked);
    QString getLineEditNewServerCloakPortText() const;
    void setLineEditNewServerCloakPortText(const QString &lineEditNewServerCloakPortText);
    QString getLineEditNewServerCloakSiteText() const;
    void setLineEditNewServerCloakSiteText(const QString &lineEditNewServerCloakSiteText);
    QString getLineEditNewServerSsPortText() const;
    void setLineEditNewServerSsPortText(const QString &lineEditNewServerSsPortText);
    QString getComboBoxNewServerSsCipherText() const;
    void setComboBoxNewServerSsCipherText(const QString &comboBoxNewServerSsCipherText);
    QString getlineEditNewServerOpenvpnPortText() const;
    void setLineEditNewServerOpenvpnPortText(const QString &lineEditNewServerOpenvpnPortText);
    QString getComboBoxNewServerOpenvpnProtoText() const;
    void setComboBoxNewServerOpenvpnProtoText(const QString &comboBoxNewServerOpenvpnProtoText);




    int getCurrentPageValue() const;
    void setCurrentPageValue(int currentPageValue);
    QString getTrayIconUrl() const;
    void setTrayIconUrl(const QString &trayIconUrl);
    bool getTrayActionDisconnectEnabled() const;
    void setTrayActionDisconnectEnabled(bool trayActionDisconnectEnabled);
    bool getTrayActionConnectEnabled() const;
    void setTrayActionConnectEnabled(bool trayActionConnectEnabled);
    bool getCheckBoxNewServerCloakChecked() const;
    void setCheckBoxNewServerCloakChecked(bool checkBoxNewServerCloakChecked);
    bool getCheckBoxNewServerSsChecked() const;
    void setCheckBoxNewServerSsChecked(bool checkBoxNewServerSsChecked);
    bool getCheckBoxNewServerOpenvpnChecked() const;
    void setCheckBoxNewServerOpenvpnChecked(bool checkBoxNewServerOpenvpnChecked);
    QString getComboBoxProtoCloakCipherText() const;
    void setComboBoxProtoCloakCipherText(const QString &comboBoxProtoCloakCipherText);
    QString getLineEditProtoCloakSiteText() const;
    void setLineEditProtoCloakSiteText(const QString &lineEditProtoCloakSiteText);
    QString getLineEditProtoCloakPortText() const;
    void setLineEditProtoCloakPortText(const QString &lineEditProtoCloakPortText);
    QString getComboBoxProtoShadowsocksCipherText() const;
    void setComboBoxProtoShadowsocksCipherText(const QString &comboBoxProtoShadowsocksCipherText);
    QString getLineEditProtoShadowsocksPortText() const;
    void setLineEditProtoShadowsocksPortText(const QString &lineEditProtoShadowsocksPortText);
    QString getLineEditProtoOpenvpnSubnetText() const;
    void setLineEditProtoOpenvpnSubnetText(const QString &lineEditProtoOpenvpnSubnetText);
    bool getRadioButtonProtoOpenvpnUdpChecked() const;
    void setRadioButtonProtoOpenvpnUdpChecked(bool radioButtonProtoOpenvpnUdpChecked);
    bool getCheckBoxProtoOpenvpnAutoEncryptionChecked() const;
    void setCheckBoxProtoOpenvpnAutoEncryptionChecked(bool checkBoxProtoOpenvpnAutoEncryptionChecked);
    QString getComboBoxProtoOpenvpnCipherText() const;
    void setComboBoxProtoOpenvpnCipherText(const QString &comboBoxProtoOpenvpnCipherText);
    QString getComboBoxProtoOpenvpnHashText() const;
    void setComboBoxProtoOpenvpnHashText(const QString &comboBoxProtoOpenvpnHashText);
    bool getCheckBoxProtoOpenvpnBlockDnsChecked() const;
    void setCheckBoxProtoOpenvpnBlockDnsChecked(bool checkBoxProtoOpenvpnBlockDnsChecked);
    QString getLineEditProtoOpenvpnPortText() const;
    void setLineEditProtoOpenvpnPortText(const QString &lineEditProtoOpenvpnPortText);
    bool getCheckBoxProtoOpenvpnTlsAuthChecked() const;
    void setCheckBoxProtoOpenvpnTlsAuthChecked(bool checkBoxProtoOpenvpnTlsAuthChecked);
    bool getRadioButtonSetupWizardHighChecked() const;
    void setRadioButtonSetupWizardHighChecked(bool radioButtonSetupWizardHighChecked);
    bool getRadioButtonSetupWizardLowChecked() const;
    void setRadioButtonSetupWizardLowChecked(bool radioButtonSetupWizardLowChecked);
    bool getCheckBoxSetupWizardVpnModeChecked() const;
    void setCheckBoxSetupWizardVpnModeChecked(bool checkBoxSetupWizardVpnModeChecked);

    bool getPushButtonConnectChecked() const;
    void setPushButtonConnectChecked(bool pushButtonConnectChecked);

    bool getWidgetProtoCloakEnabled() const;
    void setWidgetProtoCloakEnabled(bool widgetProtoCloakEnabled);
    bool getPushButtonProtoCloakSaveVisible() const;
    void setPushButtonProtoCloakSaveVisible(bool pushButtonProtoCloakSaveVisible);
    bool getProgressBarProtoCloakResetVisible() const;
    void setProgressBarProtoCloakResetVisible(bool progressBarProtoCloakResetVisible);
    bool getLineEditProtoCloakPortEnabled() const;
    void setLineEditProtoCloakPortEnabled(bool lineEditProtoCloakPortEnabled);
    bool getWidgetProtoSsEnabled() const;
    void setWidgetProtoSsEnabled(bool widgetProtoSsEnabled);
    bool getPushButtonProtoShadowsocksSaveVisible() const;
    void setPushButtonProtoShadowsocksSaveVisible(bool pushButtonProtoShadowsocksSaveVisible);
    bool getProgressBarProtoShadowsocksResetVisible() const;
    void setProgressBarProtoShadowsocksResetVisible(bool progressBarProtoShadowsocksResetVisible);
    bool getLineEditProtoShadowsocksPortEnabled() const;
    void setLineEditProtoShadowsocksPortEnabled(bool lineEditProtoShadowsocksPortEnabled);
    bool getWidgetProtoOpenvpnEnabled() const;
    void setWidgetProtoOpenvpnEnabled(bool widgetProtoOpenvpnEnabled);
    bool getPushButtonProtoOpenvpnSaveVisible() const;
    void setPushButtonProtoOpenvpnSaveVisible(bool pushButtonProtoOpenvpnSaveVisible);
    bool getProgressBarProtoOpenvpnResetVisible() const;
    void setProgressBarProtoOpenvpnResetVisible(bool progressBarProtoOpenvpnResetVisible);
    bool getRadioButtonProtoOpenvpnUdpEnabled() const;
    void setRadioButtonProtoOpenvpnUdpEnabled(bool radioButtonProtoOpenvpnUdpEnabled);
    bool getRadioButtonProtoOpenvpnTcpEnabled() const;
    void setRadioButtonProtoOpenvpnTcpEnabled(bool radioButtonProtoOpenvpnTcpEnabled);
    bool getRadioButtonProtoOpenvpnTcpChecked() const;
    void setRadioButtonProtoOpenvpnTcpChecked(bool radioButtonProtoOpenvpnTcpChecked);
    bool getLineEditProtoOpenvpnPortEnabled() const;
    void setLineEditProtoOpenvpnPortEnabled(bool lineEditProtoOpenvpnPortEnabled);
    bool getPushButtonProtoOpenvpnContInstallChecked() const;
    void setPushButtonProtoOpenvpnContInstallChecked(bool pushButtonProtoOpenvpnContInstallChecked);
    bool getPushButtonProtoSsOpenvpnContInstallChecked() const;
    void setPushButtonProtoSsOpenvpnContInstallChecked(bool pushButtonProtoSsOpenvpnContInstallChecked);
    bool getPushButtonProtoCloakOpenvpnContInstallChecked() const;
    void setPushButtonProtoCloakOpenvpnContInstallChecked(bool pushButtonProtoCloakOpenvpnContInstallChecked);
    bool getPushButtonProtoWireguardContInstallChecked() const;
    void setPushButtonProtoWireguardContInstallChecked(bool pushButtonProtoWireguardContInstallChecked);
    bool getPushButtonProtoOpenvpnContInstallEnabled() const;
    void setPushButtonProtoOpenvpnContInstallEnabled(bool pushButtonProtoOpenvpnContInstallEnabled);
    bool getPushButtonProtoSsOpenvpnContInstallEnabled() const;
    void setPushButtonProtoSsOpenvpnContInstallEnabled(bool pushButtonProtoSsOpenvpnContInstallEnabled);
    bool getPushButtonProtoCloakOpenvpnContInstallEnabled() const;
    void setPushButtonProtoCloakOpenvpnContInstallEnabled(bool pushButtonProtoCloakOpenvpnContInstallEnabled);
    bool getPushButtonProtoWireguardContInstallEnabled() const;
    void setPushButtonProtoWireguardContInstallEnabled(bool pushButtonProtoWireguardContInstallEnabled);
    bool getPushButtonProtoOpenvpnContDefaultChecked() const;
    void setPushButtonProtoOpenvpnContDefaultChecked(bool pushButtonProtoOpenvpnContDefaultChecked);
    bool getPushButtonProtoSsOpenvpnContDefaultChecked() const;
    void setPushButtonProtoSsOpenvpnContDefaultChecked(bool pushButtonProtoSsOpenvpnContDefaultChecked);
    bool getPushButtonProtoCloakOpenvpnContDefaultChecked() const;
    void setPushButtonProtoCloakOpenvpnContDefaultChecked(bool pushButtonProtoCloakOpenvpnContDefaultChecked);
    bool getPushButtonProtoWireguardContDefaultChecked() const;
    void setPushButtonProtoWireguardContDefaultChecked(bool pushButtonProtoWireguardContDefaultChecked);
    bool getPushButtonProtoOpenvpnContDefaultVisible() const;
    void setPushButtonProtoOpenvpnContDefaultVisible(bool pushButtonProtoOpenvpnContDefaultVisible);
    bool getPushButtonProtoSsOpenvpnContDefaultVisible() const;
    void setPushButtonProtoSsOpenvpnContDefaultVisible(bool pushButtonProtoSsOpenvpnContDefaultVisible);
    bool getPushButtonProtoCloakOpenvpnContDefaultVisible() const;
    void setPushButtonProtoCloakOpenvpnContDefaultVisible(bool pushButtonProtoCloakOpenvpnContDefaultVisible);
    bool getPushButtonProtoWireguardContDefaultVisible() const;
    void setPushButtonProtoWireguardContDefaultVisible(bool pushButtonProtoWireguardContDefaultVisible);
    bool getPushButtonProtoOpenvpnContShareVisible() const;
    void setPushButtonProtoOpenvpnContShareVisible(bool pushButtonProtoOpenvpnContShareVisible);
    bool getPushButtonProtoSsOpenvpnContShareVisible() const;
    void setPushButtonProtoSsOpenvpnContShareVisible(bool pushButtonProtoSsOpenvpnContShareVisible);
    bool getPushButtonProtoCloakOpenvpnContShareVisible() const;
    void setPushButtonProtoCloakOpenvpnContShareVisible(bool pushButtonProtoCloakOpenvpnContShareVisible);
    bool getPushButtonProtoWireguardContShareVisible() const;
    void setPushButtonProtoWireguardContShareVisible(bool pushButtonProtoWireguardContShareVisible);
    bool getFrameOpenvpnSettingsVisible() const;
    void setFrameOpenvpnSettingsVisible(bool frameOpenvpnSettingsVisible);
    bool getFrameOpenvpnSsSettingsVisible() const;
    void setFrameOpenvpnSsSettingsVisible(bool frameOpenvpnSsSettingsVisible);
    bool getFrameOpenvpnSsCloakSettingsVisible() const;
    void setFrameOpenvpnSsCloakSettingsVisible(bool frameOpenvpnSsCloakSettingsVisible);
    bool getProgressBarProtocolsContainerReinstallVisible() const;
    void setProgressBarProtocolsContainerReinstallVisible(bool progressBarProtocolsContainerReinstallVisible);
    QString getLabelSpeedReceivedText() const;
    void setLabelSpeedReceivedText(const QString &labelSpeedReceivedText);
    QString getLabelSpeedSentText() const;
    void setLabelSpeedSentText(const QString &labelSpeedSentText);
    QString getLabelStateText() const;
    void setLabelStateText(const QString &labelStateText);
    bool getPushButtonConnectEnabled() const;
    void setPushButtonConnectEnabled(bool pushButtonConnectEnabled);
    bool getWidgetVpnModeEnabled() const;
    void setWidgetVpnModeEnabled(bool widgetVpnModeEnabled);
    QString getLabelErrorText() const;
    void setLabelErrorText(const QString &labelErrorText);


    QString getDialogConnectErrorText() const;
    void setDialogConnectErrorText(const QString &dialogConnectErrorText);



    bool getPageNewServerConfiguringEnabled() const;
    void setPageNewServerConfiguringEnabled(bool pageNewServerConfiguringEnabled);
    bool getLabelNewServerConfiguringWaitInfoVisible() const;
    void setLabelNewServerConfiguringWaitInfoVisible(bool labelNewServerConfiguringWaitInfoVisible);
    QString getLabelNewServerConfiguringWaitInfoText() const;
    void setLabelNewServerConfiguringWaitInfoText(const QString &labelNewServerConfiguringWaitInfoText);
    bool getProgressBarNewServerConfiguringVisible() const;
    void setProgressBarNewServerConfiguringVisible(bool progressBarNewServerConfiguringVisible);
    int getProgressBarNewServerConfiguringMaximium() const;
    void setProgressBarNewServerConfiguringMaximium(int progressBarNewServerConfiguringMaximium);
    bool getProgressBarNewServerConfiguringTextVisible() const;
    void setProgressBarNewServerConfiguringTextVisible(bool progressBarNewServerConfiguringTextVisible);
    QString getProgressBarNewServerConfiguringText() const;
    void setProgressBarNewServerConfiguringText(const QString &progressBarNewServerConfiguringText);
    bool getPageServerProtocolsEnabled() const;
    void setPageServerProtocolsEnabled(bool pageServerProtocolsEnabled);
    int getProgressBarProtocolsContainerReinstallValue() const;
    void setProgressBarProtocolsContainerReinstallValue(int progressBarProtocolsContainerReinstallValue);
    int getProgressBarProtocolsContainerReinstallMaximium() const;
    void setProgressBarProtocolsContainerReinstallMaximium(int progressBarProtocolsContainerReinstallMaximium);
    bool getComboBoxProtoOpenvpnCipherEnabled() const;
    void setComboBoxProtoOpenvpnCipherEnabled(bool comboBoxProtoOpenvpnCipherEnabled);
    bool getComboBoxProtoOpenvpnHashEnabled() const;
    void setComboBoxProtoOpenvpnHashEnabled(bool comboBoxProtoOpenvpnHashEnabled);
    bool getPageProtoOpenvpnEnabled() const;
    void setPageProtoOpenvpnEnabled(bool pageProtoOpenvpnEnabled);
    bool getLabelProtoOpenvpnInfoVisible() const;
    void setLabelProtoOpenvpnInfoVisible(bool labelProtoOpenvpnInfoVisible);
    QString getLabelProtoOpenvpnInfoText() const;
    void setLabelProtoOpenvpnInfoText(const QString &labelProtoOpenvpnInfoText);
    int getProgressBarProtoOpenvpnResetValue() const;
    void setProgressBarProtoOpenvpnResetValue(int progressBarProtoOpenvpnResetValue);
    int getProgressBarProtoOpenvpnResetMaximium() const;
    void setProgressBarProtoOpenvpnResetMaximium(int progressBarProtoOpenvpnResetMaximium);
    bool getPageProtoShadowsocksEnabled() const;
    void setPageProtoShadowsocksEnabled(bool pageProtoShadowsocksEnabled);
    bool getLabelProtoShadowsocksInfoVisible() const;
    void setLabelProtoShadowsocksInfoVisible(bool labelProtoShadowsocksInfoVisible);
    QString getLabelProtoShadowsocksInfoText() const;
    void setLabelProtoShadowsocksInfoText(const QString &labelProtoShadowsocksInfoText);
    int getProgressBarProtoShadowsocksResetValue() const;
    void setProgressBarProtoShadowsocksResetValue(int progressBarProtoShadowsocksResetValue);
    int getProgressBarProtoShadowsocksResetMaximium() const;
    void setProgressBarProtoShadowsocksResetMaximium(int progressBarProtoShadowsocksResetMaximium);
    bool getPageProtoCloakEnabled() const;
    void setPageProtoCloakEnabled(bool pageProtoCloakEnabled);
    bool getLabelProtoCloakInfoVisible() const;
    void setLabelProtoCloakInfoVisible(bool labelProtoCloakInfoVisible);
    QString getLabelProtoCloakInfoText() const;
    void setLabelProtoCloakInfoText(const QString &labelProtoCloakInfoText);
    int getProgressBarProtoCloakResetValue() const;
    void setProgressBarProtoCloakResetValue(int progressBarProtoCloakResetValue);
    int getProgressBarProtoCloakResetMaximium() const;
    void setProgressBarProtoCloakResetMaximium(int progressBarProtoCloakResetMaximium);

    bool getRadioButtonVpnModeAllSitesChecked() const;
    void setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked);
    bool getRadioButtonVpnModeForwardSitesChecked() const;
    void setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked);
    bool getRadioButtonVpnModeExceptSitesChecked() const;
    void setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked);
    bool getPushButtonVpnAddSiteEnabled() const;
    void setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled);

    Q_INVOKABLE void updateWizardHighPage();
    Q_INVOKABLE void updateNewServerProtocolsPage();
    Q_INVOKABLE void updateVpnPage();


    Q_INVOKABLE void onPushButtonSetupWizardVpnModeFinishClicked();
    Q_INVOKABLE void onPushButtonSetupWizardLowFinishClicked();
    Q_INVOKABLE void onRadioButtonVpnModeAllSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeForwardSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeExceptSitesToggled(bool checked);

    Q_INVOKABLE void onPushButtonConnectClicked(bool checked);





    Q_INVOKABLE void onPushButtonProtoOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContSsConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContSsConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContCloakConfigClicked();
    Q_INVOKABLE void onCheckBoxProtoOpenvpnAutoEncryptionClicked();
    Q_INVOKABLE void onPushButtonProtoOpenvpnSaveClicked();
    Q_INVOKABLE void onPushButtonProtoShadowsocksSaveClicked();
    Q_INVOKABLE void onPushButtonProtoCloakSaveClicked();
    Q_INVOKABLE void onCloseWindow();





    Q_INVOKABLE void updateProtocolsPage();

signals:
    void frameWireguardSettingsVisibleChanged();
    void frameWireguardVisibleChanged();
    void frameNewServerSettingsParentWireguardVisibleChanged();
    void radioButtonSetupWizardMediumCheckedChanged();
    void lineEditSetupWizardHighWebsiteMaskingTextChanged();
    void progressBarNewServerConfiguringValueChanged();
    void pushButtonNewServerSettingsCloakCheckedChanged();
    void pushButtonNewServerSettingsSsCheckedChanged();
    void pushButtonNewServerSettingsOpenvpnCheckedChanged();
    void lineEditNewServerCloakPortTextChanged();
    void lineEditNewServerCloakSiteTextChanged();
    void lineEditNewServerSsPortTextChanged();
    void comboBoxNewServerSsCipherTextChanged();
    void lineEditNewServerOpenvpnPortTextChanged();
    void comboBoxNewServerOpenvpnProtoTextChanged();


    void radioButtonVpnModeAllSitesCheckedChanged();
    void radioButtonVpnModeForwardSitesCheckedChanged();
    void radioButtonVpnModeExceptSitesCheckedChanged();
    void pushButtonVpnAddSiteEnabledChanged();



    void currentPageValueChanged();
    void trayIconUrlChanged();
    void trayActionDisconnectEnabledChanged();
    void trayActionConnectEnabledChanged();
    void checkBoxNewServerCloakCheckedChanged();
    void checkBoxNewServerSsCheckedChanged();
    void checkBoxNewServerOpenvpnCheckedChanged();
    void comboBoxProtoCloakCipherTextChanged();
    void lineEditProtoCloakSiteTextChanged();
    void lineEditProtoCloakPortTextChanged();
    void comboBoxProtoShadowsocksCipherTextChanged();
    void lineEditProtoShadowsocksPortTextChanged();
    void lineEditProtoOpenvpnSubnetTextChanged();
    void radioButtonProtoOpenvpnUdpCheckedChanged();
    void checkBoxProtoOpenvpnAutoEncryptionCheckedChanged();
    void comboBoxProtoOpenvpnCipherTextChanged();
    void comboBoxProtoOpenvpnHashTextChanged();
    void checkBoxProtoOpenvpnBlockDnsCheckedChanged();
    void lineEditProtoOpenvpnPortTextChanged();
    void checkBoxProtoOpenvpnTlsAuthCheckedChanged();
    void radioButtonSetupWizardHighCheckedChanged();
    void radioButtonSetupWizardLowCheckedChanged();
    void checkBoxSetupWizardVpnModeCheckedChanged();
    void pushButtonConnectCheckedChanged();

    void widgetProtoCloakEnabledChanged();
    void pushButtonProtoCloakSaveVisibleChanged();
    void progressBarProtoCloakResetVisibleChanged();
    void lineEditProtoCloakPortEnabledChanged();
    void widgetProtoSsEnabledChanged();
    void pushButtonProtoShadowsocksSaveVisibleChanged();
    void progressBarProtoShadowsocksResetVisibleChanged();
    void lineEditProtoShadowsocksPortEnabledChanged();
    void widgetProtoOpenvpnEnabledChanged();
    void pushButtonProtoOpenvpnSaveVisibleChanged();
    void progressBarProtoOpenvpnResetVisibleChanged();
    void radioButtonProtoOpenvpnUdpEnabledChanged();
    void radioButtonProtoOpenvpnTcpEnabledChanged();
    void radioButtonProtoOpenvpnTcpCheckedChanged();
    void lineEditProtoOpenvpnPortEnabledChanged();
    void pushButtonProtoOpenvpnContInstallCheckedChanged();
    void pushButtonProtoSsOpenvpnContInstallCheckedChanged();
    void pushButtonProtoCloakOpenvpnContInstallCheckedChanged();
    void pushButtonProtoWireguardContInstallCheckedChanged();
    void pushButtonProtoOpenvpnContInstallEnabledChanged();
    void pushButtonProtoSsOpenvpnContInstallEnabledChanged();
    void pushButtonProtoCloakOpenvpnContInstallEnabledChanged();
    void pushButtonProtoWireguardContInstallEnabledChanged();
    void pushButtonProtoOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoSsOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoCloakOpenvpnContDefaultCheckedChanged();
    void pushButtonProtoWireguardContDefaultCheckedChanged();
    void pushButtonProtoOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoSsOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoCloakOpenvpnContDefaultVisibleChanged();
    void pushButtonProtoWireguardContDefaultVisibleChanged();
    void pushButtonProtoOpenvpnContShareVisibleChanged();
    void pushButtonProtoSsOpenvpnContShareVisibleChanged();
    void pushButtonProtoCloakOpenvpnContShareVisibleChanged();
    void pushButtonProtoWireguardContShareVisibleChanged();
    void frameOpenvpnSettingsVisibleChanged();
    void frameOpenvpnSsSettingsVisibleChanged();
    void frameOpenvpnSsCloakSettingsVisibleChanged();
    void progressBarProtocolsContainerReinstallVisibleChanged();
    void labelSpeedReceivedTextChanged();
    void labelSpeedSentTextChanged();
    void labelStateTextChanged();
    void pushButtonConnectEnabledChanged();
    void widgetVpnModeEnabledChanged();
    void labelErrorTextChanged();

    void dialogConnectErrorTextChanged();

    void pageNewServerConfiguringEnabledChanged();
    void labelNewServerConfiguringWaitInfoVisibleChanged();
    void labelNewServerConfiguringWaitInfoTextChanged();
    void progressBarNewServerConfiguringVisibleChanged();
    void progressBarNewServerConfiguringMaximiumChanged();
    void progressBarNewServerConfiguringTextVisibleChanged();
    void progressBarNewServerConfiguringTextChanged();
    void pageServerProtocolsEnabledChanged();
    void progressBarProtocolsContainerReinstallValueChanged();
    void progressBarProtocolsContainerReinstallMaximiumChanged();
    void comboBoxProtoOpenvpnCipherEnabledChanged();
    void comboBoxProtoOpenvpnHashEnabledChanged();
    void pageProtoOpenvpnEnabledChanged();
    void labelProtoOpenvpnInfoVisibleChanged();
    void labelProtoOpenvpnInfoTextChanged();
    void progressBarProtoOpenvpnResetValueChanged();
    void progressBarProtoOpenvpnResetMaximiumChanged();
    void pageProtoShadowsocksEnabledChanged();
    void labelProtoShadowsocksInfoVisibleChanged();
    void labelProtoShadowsocksInfoTextChanged();
    void progressBarProtoShadowsocksResetValueChanged();
    void progressBarProtoShadowsocksResetMaximiumChanged();
    void pageProtoCloakEnabledChanged();
    void labelProtoCloakInfoVisibleChanged();
    void labelProtoCloakInfoTextChanged();
    void progressBarProtoCloakResetValueChanged();
    void progressBarProtoCloakResetMaximiumChanged();

    void goToPage(int page, bool reset = true, bool slide = true);
    void closePage();
    void setStartPage(int page, bool slide = true);
    void pushButtonNewServerConnectConfigureClicked();
    void showPublicKeyWarning();
    void showConnectErrorDialog();
    void show();
    void hide();
    void pushButtonProtoOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoSsOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContDefaultClicked(bool checked);
    void pushButtonProtoWireguardContDefaultClicked(bool checked);
    void pushButtonProtoOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoSsOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContInstallClicked(bool checked);
    void pushButtonProtoWireguardContInstallClicked(bool checked);
    void pushButtonProtoOpenvpnContShareClicked(bool checked);
    void pushButtonProtoSsOpenvpnContShareClicked(bool checked);
    void pushButtonProtoCloakOpenvpnContShareClicked(bool checked);
    void pushButtonProtoWireguardContShareClicked(bool checked);

private:
    bool m_frameWireguardSettingsVisible;
    bool m_frameWireguardVisible;
    bool m_frameNewServerSettingsParentWireguardVisible;
    bool m_radioButtonSetupWizardMediumChecked;
    QString m_lineEditSetupWizardHighWebsiteMaskingText;
    double m_progressBarNewServerConfiguringValue;
    bool m_pushButtonNewServerSettingsCloakChecked;
    bool m_pushButtonNewServerSettingsSsChecked;
    bool m_pushButtonNewServerSettingsOpenvpnChecked;
    QString m_lineEditNewServerCloakPortText;
    QString m_lineEditNewServerCloakSiteText;
    QString m_lineEditNewServerSsPortText;
    QString m_comboBoxNewServerSsCipherText;
    QString m_lineEditNewServerOpenvpnPortText;
    QString m_comboBoxNewServerOpenvpnProtoText;



    bool m_radioButtonVpnModeAllSitesChecked;
    bool m_radioButtonVpnModeForwardSitesChecked;
    bool m_radioButtonVpnModeExceptSitesChecked;
    bool m_pushButtonVpnAddSiteEnabled;



    int m_currentPageValue;
    QString m_trayIconUrl;
    bool m_trayActionDisconnectEnabled;
    bool m_trayActionConnectEnabled;
    bool m_checkBoxNewServerCloakChecked;
    bool m_checkBoxNewServerSsChecked;
    bool m_checkBoxNewServerOpenvpnChecked;
    QString m_comboBoxProtoCloakCipherText;
    QString m_lineEditProtoCloakSiteText;
    QString m_lineEditProtoCloakPortText;
    QString m_comboBoxProtoShadowsocksCipherText;
    QString m_lineEditProtoShadowsocksPortText;
    QString m_lineEditProtoOpenvpnSubnetText;
    bool m_radioButtonProtoOpenvpnUdpChecked;
    bool m_checkBoxProtoOpenvpnAutoEncryptionChecked;
    QString m_comboBoxProtoOpenvpnCipherText;
    QString m_comboBoxProtoOpenvpnHashText;
    bool m_checkBoxProtoOpenvpnBlockDnsChecked;
    QString m_lineEditProtoOpenvpnPortText;
    bool m_checkBoxProtoOpenvpnTlsAuthChecked;
    bool m_radioButtonSetupWizardHighChecked;
    bool m_radioButtonSetupWizardLowChecked;
    bool m_checkBoxSetupWizardVpnModeChecked;

    bool m_pushButtonConnectChecked;

    bool m_widgetProtoCloakEnabled;
    bool m_pushButtonProtoCloakSaveVisible;
    bool m_progressBarProtoCloakResetVisible;
    bool m_lineEditProtoCloakPortEnabled;
    bool m_widgetProtoSsEnabled;
    bool m_pushButtonProtoShadowsocksSaveVisible;
    bool m_progressBarProtoShadowsocksResetVisible;
    bool m_lineEditProtoShadowsocksPortEnabled;
    bool m_widgetProtoOpenvpnEnabled;
    bool m_pushButtonProtoOpenvpnSaveVisible;
    bool m_progressBarProtoOpenvpnResetVisible;
    bool m_radioButtonProtoOpenvpnUdpEnabled;
    bool m_radioButtonProtoOpenvpnTcpEnabled;
    bool m_radioButtonProtoOpenvpnTcpChecked;
    bool m_lineEditProtoOpenvpnPortEnabled;
    bool m_pushButtonProtoOpenvpnContInstallChecked;
    bool m_pushButtonProtoSsOpenvpnContInstallChecked;
    bool m_pushButtonProtoCloakOpenvpnContInstallChecked;
    bool m_pushButtonProtoWireguardContInstallChecked;
    bool m_pushButtonProtoOpenvpnContInstallEnabled;
    bool m_pushButtonProtoSsOpenvpnContInstallEnabled;
    bool m_pushButtonProtoCloakOpenvpnContInstallEnabled;
    bool m_pushButtonProtoWireguardContInstallEnabled;
    bool m_pushButtonProtoOpenvpnContDefaultChecked;
    bool m_pushButtonProtoSsOpenvpnContDefaultChecked;
    bool m_pushButtonProtoCloakOpenvpnContDefaultChecked;
    bool m_pushButtonProtoWireguardContDefaultChecked;
    bool m_pushButtonProtoOpenvpnContDefaultVisible;
    bool m_pushButtonProtoSsOpenvpnContDefaultVisible;
    bool m_pushButtonProtoCloakOpenvpnContDefaultVisible;
    bool m_pushButtonProtoWireguardContDefaultVisible;
    bool m_pushButtonProtoOpenvpnContShareVisible;
    bool m_pushButtonProtoSsOpenvpnContShareVisible;
    bool m_pushButtonProtoCloakOpenvpnContShareVisible;
    bool m_pushButtonProtoWireguardContShareVisible;
    bool m_frameOpenvpnSettingsVisible;
    bool m_frameOpenvpnSsSettingsVisible;
    bool m_frameOpenvpnSsCloakSettingsVisible;
    bool m_progressBarProtocolsContainerReinstallVisible;
    QString m_labelSpeedReceivedText;
    QString m_labelSpeedSentText;
    QString m_labelStateText;
    bool m_pushButtonConnectEnabled;
    bool m_widgetVpnModeEnabled;
    QString m_labelErrorText;
    QString m_dialogConnectErrorText;

    bool m_pageNewServerConfiguringEnabled;
    bool m_labelNewServerConfiguringWaitInfoVisible;
    QString m_labelNewServerConfiguringWaitInfoText;
    bool m_progressBarNewServerConfiguringVisible;
    int m_progressBarNewServerConfiguringMaximium;
    bool m_progressBarNewServerConfiguringTextVisible;
    QString m_progressBarNewServerConfiguringText;
    bool m_pageServerProtocolsEnabled;
    int m_progressBarProtocolsContainerReinstallValue;
    int m_progressBarProtocolsContainerReinstallMaximium;
    bool m_comboBoxProtoOpenvpnCipherEnabled;
    bool m_comboBoxProtoOpenvpnHashEnabled;
    bool m_pageProtoOpenvpnEnabled;
    bool m_labelProtoOpenvpnInfoVisible;
    QString m_labelProtoOpenvpnInfoText;
    int m_progressBarProtoOpenvpnResetValue;
    int m_progressBarProtoOpenvpnResetMaximium;
    bool m_pageProtoShadowsocksEnabled;
    bool m_labelProtoShadowsocksInfoVisible;
    QString m_labelProtoShadowsocksInfoText;
    int m_progressBarProtoShadowsocksResetValue;
    int m_progressBarProtoShadowsocksResetMaximium;
    bool m_pageProtoCloakEnabled;
    bool m_labelProtoCloakInfoVisible;
    QString m_labelProtoCloakInfoText;
    int m_progressBarProtoCloakResetValue;
    int m_progressBarProtoCloakResetMaximium;

private slots:
    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    void onVpnProtocolError(amnezia::ErrorCode errorCode);

    void installServer(const QMap<DockerContainer, QJsonObject> &containers);
    void setTrayState(VpnProtocol::ConnectionState state);
    void onConnect();
    void onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void onDisconnect();


private:
    PageEnumNS::Page currentPage();
    struct ProgressFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(int)> setValueFunc;
        std::function<int(void)> getValueFunc;
        std::function<int(void)> getMaximiumFunc;
        std::function<void(bool)> setTextVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };
    struct PageFunc {
        std::function<void(bool)> setEnabledFunc;
    };
    struct ButtonFunc {
        std::function<void(bool)> setVisibleFunc;
    };
    struct LabelFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };

    bool installContainers(ServerCredentials credentials,
                           const QMap<DockerContainer, QJsonObject> &containers,
                           const PageFunc& page,
                           const ProgressFunc& progress,
                           const ButtonFunc& button,
                           const LabelFunc& info);

    ErrorCode doInstallAction(const std::function<ErrorCode()> &action,
                              const PageFunc& page,
                              const ProgressFunc& progress,
                              const ButtonFunc& button,
                              const LabelFunc& info);

    void setupTray();
    void setTrayIcon(const QString &iconPath);

    void setupNewServerConnections();
   // void setupSitesPageConnections();
    void setupProtocolsPageConnections();

    void updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData);
    void updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData);
    void updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData);



    QJsonObject getOpenVpnConfigFromPage(QJsonObject oldConfig);
    QJsonObject getShadowSocksConfigFromPage(QJsonObject oldConfig);
    QJsonObject getCloakConfigFromPage(QJsonObject oldConfig);

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;
    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

private:
    VpnConnection* m_vpnConnection;
    Settings m_settings;


    //    QRegExpValidator m_ipAddressValidator;
    //    QRegExpValidator m_ipAddressPortValidator;
    //    QRegExpValidator m_ipNetwok24Validator;
    //    QRegExpValidator m_ipPortValidator;

    //    QPoint offset;
    //    bool needToHideCustomTitlebar = false;

    //    void keyPressEvent(QKeyEvent* event) override;
    //    void showEvent(QShowEvent *event) override;
    //    void hideEvent(QHideEvent *event) override;

    const QString ConnectedTrayIconName = "active.png";
    const QString DisconnectedTrayIconName = "default.png";
    const QString ErrorTrayIconName = "error.png";


    //    QStack<Page> pagesStack;
    int selectedServerIndex = -1; // server index to use when proto settings page opened
    DockerContainer selectedDockerContainer; // same
    ServerCredentials installCredentials; // used to save cred between pages new_server and new_server_protocols and wizard
};
#endif // UILOGIC_H
