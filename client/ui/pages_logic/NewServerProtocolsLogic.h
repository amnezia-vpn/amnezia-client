#ifndef NEW_SERVER_PROTOCOLS_LOGIC_H
#define NEW_SERVER_PROTOCOLS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class NewServerProtocolsLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateNewServerProtocolsPage();

    Q_PROPERTY(bool frameNewServerSettingsParentWireguardVisible READ getFrameNewServerSettingsParentWireguardVisible WRITE setFrameNewServerSettingsParentWireguardVisible NOTIFY frameNewServerSettingsParentWireguardVisibleChanged)
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
    Q_PROPERTY(double progressBarNewServerConnectionMinimum READ getProgressBarNewServerConnectionMinimum WRITE setProgressBarNewServerConnectionMinimum NOTIFY progressBarNewServerConnectionMinimumChanged)
    Q_PROPERTY(double progressBarNewServerConnectionMaximum READ getProgressBarNewServerConnectionMaximum WRITE setProgressBarNewServerConnectionMaximum NOTIFY progressBarNewServerConnectionMaximumChanged)

public:
    explicit NewServerProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerProtocolsLogic() = default;

    QMap<DockerContainer, QJsonObject> getInstallConfigsFromProtocolsPage() const;

    bool getFrameNewServerSettingsParentWireguardVisible() const;
    void setFrameNewServerSettingsParentWireguardVisible(bool frameNewServerSettingsParentWireguardVisible);

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

    bool getCheckBoxNewServerCloakChecked() const;
    void setCheckBoxNewServerCloakChecked(bool checkBoxNewServerCloakChecked);
    bool getCheckBoxNewServerSsChecked() const;
    void setCheckBoxNewServerSsChecked(bool checkBoxNewServerSsChecked);
    bool getCheckBoxNewServerOpenvpnChecked() const;
    void setCheckBoxNewServerOpenvpnChecked(bool checkBoxNewServerOpenvpnChecked);

    double getProgressBarNewServerConnectionMinimum() const;
    void setProgressBarNewServerConnectionMinimum(double progressBarNewServerConnectionMinimum);
    double getProgressBarNewServerConnectionMaximum() const;
    void setProgressBarNewServerConnectionMaximum(double progressBarNewServerConnectionMaximum);

signals:
    void frameNewServerSettingsParentWireguardVisibleChanged();
    void pushButtonNewServerConnectConfigureClicked();

    void pushButtonNewServerSettingsCloakCheckedChanged();
    void pushButtonNewServerSettingsSsCheckedChanged();
    void pushButtonNewServerSettingsOpenvpnCheckedChanged();
    void lineEditNewServerCloakPortTextChanged();
    void lineEditNewServerCloakSiteTextChanged();
    void lineEditNewServerSsPortTextChanged();
    void comboBoxNewServerSsCipherTextChanged();
    void lineEditNewServerOpenvpnPortTextChanged();
    void comboBoxNewServerOpenvpnProtoTextChanged();

    void checkBoxNewServerCloakCheckedChanged();
    void checkBoxNewServerSsCheckedChanged();
    void checkBoxNewServerOpenvpnCheckedChanged();

    void progressBarNewServerConnectionMinimumChanged();
    void progressBarNewServerConnectionMaximumChanged();

private:


private slots:


private:
    bool m_frameNewServerSettingsParentWireguardVisible;

    bool m_pushButtonNewServerSettingsCloakChecked;
    bool m_pushButtonNewServerSettingsSsChecked;
    bool m_pushButtonNewServerSettingsOpenvpnChecked;
    QString m_lineEditNewServerCloakPortText;
    QString m_lineEditNewServerCloakSiteText;
    QString m_lineEditNewServerSsPortText;
    QString m_comboBoxNewServerSsCipherText;
    QString m_lineEditNewServerOpenvpnPortText;
    QString m_comboBoxNewServerOpenvpnProtoText;

    bool m_checkBoxNewServerCloakChecked;
    bool m_checkBoxNewServerSsChecked;
    bool m_checkBoxNewServerOpenvpnChecked;

    double m_progressBarNewServerConnectionMinimum;
    double m_progressBarNewServerConnectionMaximum;
};
#endif // NEW_SERVER_PROTOCOLS_LOGIC_H
