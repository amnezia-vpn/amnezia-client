#ifndef UILOGIC_H
#define UILOGIC_H

#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QRegExpValidator>
#include <QStack>
#include <QStringListModel>
#include <QSystemTrayIcon>
#include <QQmlEngine>
#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

#include "framelesswindow.h"
#include "protocols/vpnprotocol.h"

#include "settings.h"
#include "sites_model.h"

class VpnConnection;

class UiLogic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool frameWireguardSettingsVisible READ getFrameWireguardSettingsVisible WRITE setFrameWireguardSettingsVisible NOTIFY frameWireguardSettingsVisibleChanged)
    Q_PROPERTY(bool frameFireguardVisible READ getFrameFireguardVisible WRITE setFrameFireguardVisible NOTIFY frameFireguardVisibleChanged)
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
    Q_PROPERTY(bool pushButtonNewServerConnectKeyChecked READ getPushButtonNewServerConnectKeyChecked WRITE setPushButtonNewServerConnectKeyChecked NOTIFY pushButtonNewServerConnectKeyCheckedChanged)
    Q_PROPERTY(QString lineEditStartExistingCodeText READ getLineEditStartExistingCodeText WRITE setLineEditStartExistingCodeText NOTIFY lineEditStartExistingCodeTextChanged)
    Q_PROPERTY(QString textEditNewServerSshKeyText READ getTextEditNewServerSshKeyText WRITE setTextEditNewServerSshKeyText NOTIFY textEditNewServerSshKeyTextChanged)
    Q_PROPERTY(QString lineEditNewServerIpText READ getLineEditNewServerIpText WRITE setLineEditNewServerIpText NOTIFY lineEditNewServerIpTextChanged)
    Q_PROPERTY(QString lineEditNewServerPasswordText READ getLineEditNewServerPasswordText WRITE setLineEditNewServerPasswordText NOTIFY lineEditNewServerPasswordTextChanged)
    Q_PROPERTY(QString lineEditNewServerLoginText READ getLineEditNewServerLoginText WRITE setLineEditNewServerLoginText NOTIFY lineEditNewServerLoginTextChanged)
    Q_PROPERTY(bool labelNewServerWaitInfoVisible READ getLabelNewServerWaitInfoVisible WRITE setLabelNewServerWaitInfoVisible NOTIFY labelNewServerWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelNewServerWaitInfoText READ getLabelNewServerWaitInfoText WRITE setLabelNewServerWaitInfoText NOTIFY labelNewServerWaitInfoTextChanged)
    Q_PROPERTY(double progressBarNewServerConnectionMinimum READ getProgressBarNewServerConnectionMinimum WRITE setProgressBarNewServerConnectionMinimum NOTIFY progressBarNewServerConnectionMinimumChanged)
    Q_PROPERTY(double progressBarNewServerConnectionMaximum READ getProgressBarNewServerConnectionMaximum WRITE setProgressBarNewServerConnectionMaximum NOTIFY progressBarNewServerConnectionMaximumChanged)
    Q_PROPERTY(bool pushButtonBackFromStartVisible READ getPushButtonBackFromStartVisible WRITE setPushButtonBackFromStartVisible NOTIFY pushButtonBackFromStartVisibleChanged)
    Q_PROPERTY(bool pushButtonNewServerConnectVisible READ getPushButtonNewServerConnectVisible WRITE setPushButtonNewServerConnectVisible NOTIFY pushButtonNewServerConnectVisibleChanged)
    Q_PROPERTY(bool radioButtonVpnModeAllSitesChecked READ getRadioButtonVpnModeAllSitesChecked WRITE setRadioButtonVpnModeAllSitesChecked NOTIFY radioButtonVpnModeAllSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeForwardSitesChecked READ getRadioButtonVpnModeForwardSitesChecked WRITE setRadioButtonVpnModeForwardSitesChecked NOTIFY radioButtonVpnModeForwardSitesCheckedChanged)
    Q_PROPERTY(bool radioButtonVpnModeExceptSitesChecked READ getRadioButtonVpnModeExceptSitesChecked WRITE setRadioButtonVpnModeExceptSitesChecked NOTIFY radioButtonVpnModeExceptSitesCheckedChanged)
    Q_PROPERTY(bool pushButtonVpnAddSiteEnabled READ getPushButtonVpnAddSiteEnabled WRITE setPushButtonVpnAddSiteEnabled NOTIFY pushButtonVpnAddSiteEnabledChanged)
    Q_PROPERTY(bool checkBoxAppSettingsAutostartChecked READ getCheckBoxAppSettingsAutostartChecked WRITE setCheckBoxAppSettingsAutostartChecked NOTIFY checkBoxAppSettingsAutostartCheckedChanged)
    Q_PROPERTY(bool checkBoxAppSettingsAutoconnectChecked READ getCheckBoxAppSettingsAutoconnectChecked WRITE setCheckBoxAppSettingsAutoconnectChecked NOTIFY checkBoxAppSettingsAutoconnectCheckedChanged)
    Q_PROPERTY(bool checkBoxAppSettingsStartMinimizedChecked READ getCheckBoxAppSettingsStartMinimizedChecked WRITE setCheckBoxAppSettingsStartMinimizedChecked NOTIFY checkBoxAppSettingsStartMinimizedCheckedChanged)
    Q_PROPERTY(QString lineEditNetworkSettingsDns1Text READ getLineEditNetworkSettingsDns1Text WRITE setLineEditNetworkSettingsDns1Text NOTIFY lineEditNetworkSettingsDns1TextChanged)
    Q_PROPERTY(QString lineEditNetworkSettingsDns2Text READ getLineEditNetworkSettingsDns2Text WRITE setLineEditNetworkSettingsDns2Text NOTIFY lineEditNetworkSettingsDns2TextChanged)
    Q_PROPERTY(QString labelAppSettingsVersionText READ getLabelAppSettingsVersionText WRITE setLabelAppSettingsVersionText NOTIFY labelAppSettingsVersionTextChanged)
    Q_PROPERTY(bool pushButtonGeneralSettingsShareConnectionEnable READ getPushButtonGeneralSettingsShareConnectionEnable WRITE setPushButtonGeneralSettingsShareConnectionEnable NOTIFY pushButtonGeneralSettingsShareConnectionEnableChanged)
    Q_PROPERTY(bool labelServerSettingsWaitInfoVisible READ getLabelServerSettingsWaitInfoVisible WRITE setLabelServerSettingsWaitInfoVisible NOTIFY labelServerSettingsWaitInfoVisibleChanged)
    Q_PROPERTY(QString labelServerSettingsWaitInfoText READ getLabelServerSettingsWaitInfoText WRITE setLabelServerSettingsWaitInfoText NOTIFY labelServerSettingsWaitInfoTextChanged)
    Q_PROPERTY(bool pushButtonServerSettingsClearVisible READ getPushButtonServerSettingsClearVisible WRITE setPushButtonServerSettingsClearVisible NOTIFY pushButtonServerSettingsClearVisibleChanged)
    Q_PROPERTY(bool pushButtonServerSettingsClearClientCacheVisible READ getPushButtonServerSettingsClearClientCacheVisible WRITE setPushButtonServerSettingsClearClientCacheVisible NOTIFY pushButtonServerSettingsClearClientCacheVisibleChanged)
    Q_PROPERTY(bool pushButtonServerSettingsShareFullVisible READ getPushButtonServerSettingsShareFullVisible WRITE setPushButtonServerSettingsShareFullVisible NOTIFY pushButtonServerSettingsShareFullVisibleChanged)
    Q_PROPERTY(QString labelServerSettingsServerText READ getLabelServerSettingsServerText WRITE setLabelServerSettingsServerText NOTIFY labelServerSettingsServerTextChanged)
    Q_PROPERTY(QString lineEditServerSettingsDescriptionText READ getLineEditServerSettingsDescriptionText WRITE setLineEditServerSettingsDescriptionText NOTIFY lineEditServerSettingsDescriptionTextChanged)
    Q_PROPERTY(QString labelServerSettingsCurrentVpnProtocolText READ getLabelServerSettingsCurrentVpnProtocolText WRITE setLabelServerSettingsCurrentVpnProtocolText NOTIFY labelServerSettingsCurrentVpnProtocolTextChanged)
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
    Q_PROPERTY(QString ipAddressValidatorRegex READ getIpAddressValidatorRegex CONSTANT)
    Q_PROPERTY(bool pushButtonConnectChecked READ getPushButtonConnectChecked WRITE setPushButtonConnectChecked NOTIFY pushButtonConnectCheckedChanged)


public:
    explicit UiLogic(QObject *parent = nullptr);
    //    ~UiLogic();

    enum Page {Start, NewServer, NewServerProtocols, Vpn,
               Wizard, WizardLow, WizardMedium, WizardHigh, WizardVpnMode, ServerConfiguring,
               GeneralSettings, AppSettings, NetworkSettings, ServerSettings,
               ServerVpnProtocols, ServersList, ShareConnection,  Sites,
               OpenVpnSettings, ShadowSocksSettings, CloakSettings};
    Q_ENUM(Page)

    //    void showOnStartup();

    Q_INVOKABLE void initalizeUiLogic();
    static void declareQML() {
        qmlRegisterType<UiLogic>("Page", 1, 0, "Style");
    }
    bool getFrameWireguardSettingsVisible() const;
    void setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible);
    bool getFrameFireguardVisible() const;
    void setFrameFireguardVisible(bool frameFireguardVisible);
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
    QString getLineEditStartExistingCodeText() const;
    void setLineEditStartExistingCodeText(const QString &lineEditStartExistingCodeText);
    QString getTextEditNewServerSshKeyText() const;
    void setTextEditNewServerSshKeyText(const QString &textEditNewServerSshKeyText);
    QString getLineEditNewServerIpText() const;
    void setLineEditNewServerIpText(const QString &lineEditNewServerIpText);
    QString getLineEditNewServerPasswordText() const;
    void setLineEditNewServerPasswordText(const QString &lineEditNewServerPasswordText);
    QString getLineEditNewServerLoginText() const;
    void setLineEditNewServerLoginText(const QString &lineEditNewServerLoginText);
    bool getLabelNewServerWaitInfoVisible() const;
    void setLabelNewServerWaitInfoVisible(bool labelNewServerWaitInfoVisible);
    QString getLabelNewServerWaitInfoText() const;
    void setLabelNewServerWaitInfoText(const QString &labelNewServerWaitInfoText);
    double getProgressBarNewServerConnectionMinimum() const;
    void setProgressBarNewServerConnectionMinimum(double progressBarNewServerConnectionMinimum);
    double getProgressBarNewServerConnectionMaximum() const;
    void setProgressBarNewServerConnectionMaximum(double progressBarNewServerConnectionMaximum);
    bool getPushButtonBackFromStartVisible() const;
    void setPushButtonBackFromStartVisible(bool pushButtonBackFromStartVisible);
    bool getPushButtonNewServerConnectVisible() const;
    void setPushButtonNewServerConnectVisible(bool pushButtonNewServerConnectVisible);
    bool getPushButtonNewServerConnectKeyChecked() const;
    void setPushButtonNewServerConnectKeyChecked(bool pushButtonNewServerConnectKeyChecked);
    bool getRadioButtonVpnModeAllSitesChecked() const;
    void setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked);
    bool getRadioButtonVpnModeForwardSitesChecked() const;
    void setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked);
    bool getRadioButtonVpnModeExceptSitesChecked() const;
    void setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked);
    bool getPushButtonVpnAddSiteEnabled() const;
    void setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled);
    bool getCheckBoxAppSettingsAutostartChecked() const;
    void setCheckBoxAppSettingsAutostartChecked(bool checkBoxAppSettingsAutostartChecked);
    bool getCheckBoxAppSettingsAutoconnectChecked() const;
    void setCheckBoxAppSettingsAutoconnectChecked(bool checkBoxAppSettingsAutoconnectChecked);
    bool getCheckBoxAppSettingsStartMinimizedChecked() const;
    void setCheckBoxAppSettingsStartMinimizedChecked(bool checkBoxAppSettingsStartMinimizedChecked);
    QString getLineEditNetworkSettingsDns1Text() const;
    void setLineEditNetworkSettingsDns1Text(const QString &lineEditNetworkSettingsDns1Text);
    QString getLineEditNetworkSettingsDns2Text() const;
    void setLineEditNetworkSettingsDns2Text(const QString &lineEditNetworkSettingsDns2Text);
    QString getLabelAppSettingsVersionText() const;
    void setLabelAppSettingsVersionText(const QString &labelAppSettingsVersionText);
    bool getPushButtonGeneralSettingsShareConnectionEnable() const;
    void setPushButtonGeneralSettingsShareConnectionEnable(bool pushButtonGeneralSettingsShareConnectionEnable);
    bool getLabelServerSettingsWaitInfoVisible() const;
    void setLabelServerSettingsWaitInfoVisible(bool labelServerSettingsWaitInfoVisible);
    QString getLabelServerSettingsWaitInfoText() const;
    void setLabelServerSettingsWaitInfoText(const QString &labelServerSettingsWaitInfoText);
    bool getPushButtonServerSettingsClearVisible() const;
    void setPushButtonServerSettingsClearVisible(bool pushButtonServerSettingsClearVisible);
    bool getPushButtonServerSettingsClearClientCacheVisible() const;
    void setPushButtonServerSettingsClearClientCacheVisible(bool pushButtonServerSettingsClearClientCacheVisible);
    bool getPushButtonServerSettingsShareFullVisible() const;
    void setPushButtonServerSettingsShareFullVisible(bool pushButtonServerSettingsShareFullVisible);
    QString getLabelServerSettingsServerText() const;
    void setLabelServerSettingsServerText(const QString &labelServerSettingsServerText);
    QString getLineEditServerSettingsDescriptionText() const;
    void setLineEditServerSettingsDescriptionText(const QString &lineEditServerSettingsDescriptionText);
    QString getLabelServerSettingsCurrentVpnProtocolText() const;
    void setLabelServerSettingsCurrentVpnProtocolText(const QString &labelServerSettingsCurrentVpnProtocolText);
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
    QString getIpAddressValidatorRegex() const;
    bool getPushButtonConnectChecked() const;
    void setPushButtonConnectChecked(bool pushButtonConnectChecked);


    Q_INVOKABLE void updateWizardHighPage();
    Q_INVOKABLE void updateNewServerProtocolsPage();
    Q_INVOKABLE void updateStartPage();
    Q_INVOKABLE void updateVpnPage();
    Q_INVOKABLE void updateAppSettingsPage();
    Q_INVOKABLE void updateGeneralSettingPage();
    Q_INVOKABLE void updateServerPage();

    Q_INVOKABLE void onPushButtonNewServerConnect();
    Q_INVOKABLE void onPushButtonNewServerImport();
    Q_INVOKABLE void onPushButtonSetupWizardVpnModeFinishClicked();
    Q_INVOKABLE void onPushButtonSetupWizardLowFinishClicked();
    Q_INVOKABLE void onRadioButtonVpnModeAllSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeForwardSitesToggled(bool checked);
    Q_INVOKABLE void onRadioButtonVpnModeExceptSitesToggled(bool checked);
    Q_INVOKABLE void onPushButtonAppSettingsOpenLogsChecked();
    Q_INVOKABLE void onCheckBoxAppSettingsAutostartToggled(bool checked);
    Q_INVOKABLE void onCheckBoxAppSettingsAutoconnectToggled(bool checked);
    Q_INVOKABLE void onCheckBoxAppSettingsStartMinimizedToggled(bool checked);
    Q_INVOKABLE void onLineEditNetworkSettingsDns1EditFinished(const QString& text);
    Q_INVOKABLE void onLineEditNetworkSettingsDns2EditFinished(const QString& text);
    Q_INVOKABLE void onPushButtonNetworkSettingsResetdns1Clicked();
    Q_INVOKABLE void onPushButtonNetworkSettingsResetdns2Clicked();
    Q_INVOKABLE void onPushButtonConnectClicked(bool checked);



signals:
    void frameWireguardSettingsVisibleChanged();
    void frameFireguardVisibleChanged();
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
    void pushButtonNewServerConnectKeyCheckedChanged();
    void lineEditStartExistingCodeTextChanged();
    void textEditNewServerSshKeyTextChanged();
    void lineEditNewServerIpTextChanged();
    void lineEditNewServerPasswordTextChanged();
    void lineEditNewServerLoginTextChanged();
    void labelNewServerWaitInfoVisibleChanged();
    void labelNewServerWaitInfoTextChanged();
    void progressBarNewServerConnectionMinimumChanged();
    void progressBarNewServerConnectionMaximumChanged();
    void pushButtonBackFromStartVisibleChanged();
    void pushButtonNewServerConnectVisibleChanged();
    void radioButtonVpnModeAllSitesCheckedChanged();
    void radioButtonVpnModeForwardSitesCheckedChanged();
    void radioButtonVpnModeExceptSitesCheckedChanged();
    void pushButtonVpnAddSiteEnabledChanged();
    void checkBoxAppSettingsAutostartCheckedChanged();
    void checkBoxAppSettingsAutoconnectCheckedChanged();
    void checkBoxAppSettingsStartMinimizedCheckedChanged();
    void lineEditNetworkSettingsDns1TextChanged();
    void lineEditNetworkSettingsDns2TextChanged();
    void labelAppSettingsVersionTextChanged();
    void pushButtonGeneralSettingsShareConnectionEnableChanged();
    void labelServerSettingsWaitInfoVisibleChanged();
    void labelServerSettingsWaitInfoTextChanged();
    void pushButtonServerSettingsClearVisibleChanged();
    void pushButtonServerSettingsClearClientCacheVisibleChanged();
    void pushButtonServerSettingsShareFullVisibleChanged();
    void labelServerSettingsServerTextChanged();
    void lineEditServerSettingsDescriptionTextChanged();
    void labelServerSettingsCurrentVpnProtocolTextChanged();
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

    void goToPage(Page page, bool reset = true, bool slide = true);
    void closePage();
    void setStartPage(Page page, bool slide = true);
    void pushButtonNewServerConnectConfigureClicked();

private:
    bool m_frameWireguardSettingsVisible;
    bool m_frameFireguardVisible;
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
    bool m_pushButtonNewServerConnectKeyChecked;
    QString m_lineEditStartExistingCodeText;
    QString m_textEditNewServerSshKeyText;
    QString m_lineEditNewServerIpText;
    QString m_lineEditNewServerPasswordText;
    QString m_lineEditNewServerLoginText;
    bool m_labelNewServerWaitInfoVisible;
    QString m_labelNewServerWaitInfoText;
    double m_progressBarNewServerConnectionMinimum;
    double m_progressBarNewServerConnectionMaximum;
    bool m_pushButtonBackFromStartVisible;
    bool m_pushButtonNewServerConnectVisible;
    bool m_radioButtonVpnModeAllSitesChecked;
    bool m_radioButtonVpnModeForwardSitesChecked;
    bool m_radioButtonVpnModeExceptSitesChecked;
    bool m_pushButtonVpnAddSiteEnabled;
    bool m_checkBoxAppSettingsAutostartChecked;
    bool m_checkBoxAppSettingsAutoconnectChecked;
    bool m_checkBoxAppSettingsStartMinimizedChecked;
    QString m_lineEditNetworkSettingsDns1Text;
    QString m_lineEditNetworkSettingsDns2Text;
    QString m_labelAppSettingsVersionText;
    bool m_pushButtonGeneralSettingsShareConnectionEnable;
    bool m_labelServerSettingsWaitInfoVisible;
    QString m_labelServerSettingsWaitInfoText;
    bool m_pushButtonServerSettingsClearVisible;
    bool m_pushButtonServerSettingsClearClientCacheVisible;
    bool m_pushButtonServerSettingsShareFullVisible;
    QString m_labelServerSettingsServerText;
    QString m_lineEditServerSettingsDescriptionText;
    QString m_labelServerSettingsCurrentVpnProtocolText;
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
    QString m_ipAddressValidatorRegex;
    bool m_pushButtonConnectChecked;

    //private slots:
    //    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    //    void onConnectionStateChanged(VpnProtocol::ConnectionState state);
    //    void onVpnProtocolError(amnezia::ErrorCode errorCode);

    void installServer(const QMap<DockerContainer, QJsonObject> &containers);

    //    void onPushButtonClearServer(bool);
    //    void onPushButtonForgetServer(bool);

    //    void onPushButtonAddCustomSitesClicked();

    void setTrayState(VpnProtocol::ConnectionState state);


    void onConnect();
    //    void onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void onDisconnect();


private:

    Page currentPage();

    //    bool installContainers(ServerCredentials credentials, const QMap<DockerContainer, QJsonObject> &containers,
    //        QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info);

    //    ErrorCode doInstallAction(const std::function<ErrorCode()> &action, QWidget *page, QProgressBar *progress, QPushButton *button, QLabel *info);

    void setupTray();
    void setTrayIcon(const QString &iconPath);

    void setupNewServerConnections();
    //        void setupSitesPageConnections();
    //        void setupGeneralSettingsConnections();
    //    void setupProtocolsPageConnections();
    void setupNewServerPageConnections();
    //    void setupServerSettingsPageConnections();
    //    void setupSharePageConnections();

    //    void updateSitesPage();
    //    void updateServersListPage();
    //    void updateProtocolsPage();
    //    void updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData);
    //    void updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData);
    //    void updateCloakPage(const QJsonObject &ckConfig, DockerContainer container, bool haveAuthData);

    //    void updateSharingPage(int serverIndex, const ServerCredentials &credentials,
    //        DockerContainer container);

    //    void makeServersListItem(QListWidget* listWidget, const QJsonObject &server, bool isDefault, int index);

    //    void updateQRCodeImage(const QString &text, QLabel *label);

    QJsonObject getOpenVpnConfigFromPage(QJsonObject oldConfig);
    QJsonObject getShadowSocksConfigFromPage(QJsonObject oldConfig);
    QJsonObject getCloakConfigFromPage(QJsonObject oldConfig);

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;
    QMap<DockerContainer, QJsonObject> getInstallConfigsFromWizardPage() const;

private:
    VpnConnection* m_vpnConnection;
    Settings m_settings;

    //    QMap<Settings::RouteMode, SitesModel *> sitesModels;

    //    QRegExpValidator m_ipAddressValidator;
    //    QRegExpValidator m_ipAddressPortValidator;
    //    QRegExpValidator m_ipNetwok24Validator;
    //    QRegExpValidator m_ipPortValidator;

    //    CQR_Encode m_qrEncode;

    //    bool canMove = false;
    //    QPoint offset;
    //    bool needToHideCustomTitlebar = false;

    //    bool eventFilter(QObject *obj, QEvent *event) override;
    //    void keyPressEvent(QKeyEvent* event) override;
    //    void closeEvent(QCloseEvent *event) override;
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
