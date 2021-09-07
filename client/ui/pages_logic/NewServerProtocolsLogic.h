#ifndef NEW_SERVER_PROTOCOLS_LOGIC_H
#define NEW_SERVER_PROTOCOLS_LOGIC_H

#include "../pages.h"
#include "settings.h"

class UiLogic;

class NewServerProtocolsLogic : public QObject
{
    Q_OBJECT

public:
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
    Q_PROPERTY(bool checkBoxNewServerCloakChecked READ getCheckBoxNewServerCloakChecked WRITE setCheckBoxNewServerCloakChecked NOTIFY checkBoxNewServerCloakCheckedChanged)
    Q_PROPERTY(bool checkBoxNewServerSsChecked READ getCheckBoxNewServerSsChecked WRITE setCheckBoxNewServerSsChecked NOTIFY checkBoxNewServerSsCheckedChanged)
    Q_PROPERTY(bool checkBoxNewServerOpenvpnChecked READ getCheckBoxNewServerOpenvpnChecked WRITE setCheckBoxNewServerOpenvpnChecked NOTIFY checkBoxNewServerOpenvpnCheckedChanged)

public:
    explicit NewServerProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerProtocolsLogic() = default;


signals:


private:


private slots:



private:
    Settings m_settings;
    UiLogic *m_uiLogic;



};
#endif // NEW_SERVER_PROTOCOLS_LOGIC_H
