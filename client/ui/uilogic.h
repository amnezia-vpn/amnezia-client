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

class OpenVpnLogic;
class ShadowSocksLogic;
class CloakLogic;

class VpnConnection;


class UiLogic : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool frameWireguardSettingsVisible READ getFrameWireguardSettingsVisible WRITE setFrameWireguardSettingsVisible NOTIFY frameWireguardSettingsVisibleChanged)
    Q_PROPERTY(bool frameWireguardVisible READ getFrameWireguardVisible WRITE setFrameWireguardVisible NOTIFY frameWireguardVisibleChanged)
    Q_PROPERTY(bool frameNewServerSettingsParentWireguardVisible READ getFrameNewServerSettingsParentWireguardVisible WRITE setFrameNewServerSettingsParentWireguardVisible NOTIFY frameNewServerSettingsParentWireguardVisibleChanged)
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

    Q_PROPERTY(bool pushButtonConnectChecked READ getPushButtonConnectChecked WRITE setPushButtonConnectChecked NOTIFY pushButtonConnectCheckedChanged)
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

    friend class OpenVpnLogic;
    friend class ShadowSocksLogic;
    friend class CloakLogic;

    Q_INVOKABLE void initalizeUiLogic();


    bool getFrameWireguardSettingsVisible() const;
    void setFrameWireguardSettingsVisible(bool frameWireguardSettingsVisible);
    bool getFrameWireguardVisible() const;
    void setFrameWireguardVisible(bool frameWireguardVisible);
    bool getFrameNewServerSettingsParentWireguardVisible() const;
    void setFrameNewServerSettingsParentWireguardVisible(bool frameNewServerSettingsParentWireguardVisible);
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


    bool getPushButtonConnectChecked() const;
    void setPushButtonConnectChecked(bool pushButtonConnectChecked);


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


    bool getRadioButtonVpnModeAllSitesChecked() const;
    void setRadioButtonVpnModeAllSitesChecked(bool radioButtonVpnModeAllSitesChecked);
    bool getRadioButtonVpnModeForwardSitesChecked() const;
    void setRadioButtonVpnModeForwardSitesChecked(bool radioButtonVpnModeForwardSitesChecked);
    bool getRadioButtonVpnModeExceptSitesChecked() const;
    void setRadioButtonVpnModeExceptSitesChecked(bool radioButtonVpnModeExceptSitesChecked);
    bool getPushButtonVpnAddSiteEnabled() const;
    void setPushButtonVpnAddSiteEnabled(bool pushButtonVpnAddSiteEnabled);

    Q_INVOKABLE void updateNewServerProtocolsPage();
    Q_INVOKABLE void updateVpnPage();

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


    Q_INVOKABLE void onCloseWindow();

    Q_INVOKABLE void updateProtocolsPage();

signals:
    void frameWireguardSettingsVisibleChanged();
    void frameWireguardVisibleChanged();
    void frameNewServerSettingsParentWireguardVisibleChanged();

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





    void pushButtonConnectCheckedChanged();


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





    bool m_pushButtonConnectChecked;



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





    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;

public:
    AppSettingsLogic *appSettingsLogic()                { return m_appSettingsLogic; }
    GeneralSettingsLogic *generalSettingsLogic()        { return m_generalSettingsLogic; }
    NetworkSettingsLogic *networkSettingsLogic()        { return m_networkSettingsLogic; }
    NewServerLogic *newServerLogic()                    { return m_newServerLogic; }
    ProtocolSettingsLogic *protocolSettingsLogic()      { return m_protocolSettingsLogic; }
    ServerListLogic *serverListLogic()                  { return m_serverListLogic; }
    ServerSettingsLogic *serverSettingsLogic()          { return m_serverSettingsLogic; }
    ServerVpnProtocolsLogic *serverVpnProtocolsLogic()  { return m_serverVpnProtocolsLogic; }
    ShareConnectionLogic *shareConnectionLogic()        { return m_shareConnectionLogic; }
    SitesLogic *sitesLogic()                            { return m_sitesLogic; }
    StartPageLogic *startPageLogic()                    { return m_startPageLogic; }
    VpnLogic *vpnLogic()                                { return m_vpnLogic; }
    WizardLogic *wizardLogic()                          { return m_wizardLogic; }

    OpenVpnLogic *openVpnLogic()                        { return m_openVpnLogic; }
    ShadowSocksLogic *shadowSocksLogic()                { return m_shadowSocksLogic; }
    CloakLogic *cloakLogic()                            { return m_cloakLogic; }

private:
    AppSettingsLogic *m_appSettingsLogic;
    GeneralSettingsLogic *m_generalSettingsLogic;
    NetworkSettingsLogic *m_networkSettingsLogic;
    NewServerLogic *m_newServerLogic;
    ProtocolSettingsLogic *m_protocolSettingsLogic;
    ServerListLogic *m_serverListLogic;
    ServerSettingsLogic *m_serverSettingsLogic;
    ServerVpnProtocolsLogic *m_serverVpnProtocolsLogic;
    ShareConnectionLogic *m_shareConnectionLogic;
    SitesLogic *m_sitesLogic;
    StartPageLogic *m_startPageLogic;
    VpnLogic *m_vpnLogic;
    WizardLogic *m_wizardLogic;

    OpenVpnLogic *m_openVpnLogic;
    ShadowSocksLogic *m_shadowSocksLogic;
    CloakLogic *m_cloakLogic;

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
