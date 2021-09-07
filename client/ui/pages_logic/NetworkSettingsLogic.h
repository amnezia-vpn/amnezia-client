#ifndef NETWORK_SETTINGS_LOGIC_H
#define NETWORK_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class NetworkSettingsLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateNetworkSettingsPage();

    Q_PROPERTY(QString lineEditNetworkSettingsDns1Text READ getLineEditNetworkSettingsDns1Text WRITE setLineEditNetworkSettingsDns1Text NOTIFY lineEditNetworkSettingsDns1TextChanged)
    Q_PROPERTY(QString lineEditNetworkSettingsDns2Text READ getLineEditNetworkSettingsDns2Text WRITE setLineEditNetworkSettingsDns2Text NOTIFY lineEditNetworkSettingsDns2TextChanged)

    Q_PROPERTY(QString ipAddressValidatorRegex READ getIpAddressValidatorRegex CONSTANT)

    Q_INVOKABLE void onLineEditNetworkSettingsDns1EditFinished(const QString& text);
    Q_INVOKABLE void onLineEditNetworkSettingsDns2EditFinished(const QString& text);
    Q_INVOKABLE void onPushButtonNetworkSettingsResetdns1Clicked();
    Q_INVOKABLE void onPushButtonNetworkSettingsResetdns2Clicked();

public:
    explicit NetworkSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NetworkSettingsLogic() = default;



    QString getLineEditNetworkSettingsDns1Text() const;
    void setLineEditNetworkSettingsDns1Text(const QString &lineEditNetworkSettingsDns1Text);
    QString getLineEditNetworkSettingsDns2Text() const;
    void setLineEditNetworkSettingsDns2Text(const QString &lineEditNetworkSettingsDns2Text);

    QString getIpAddressValidatorRegex() const;

signals:
    void lineEditNetworkSettingsDns1TextChanged();
    void lineEditNetworkSettingsDns2TextChanged();



private:


private slots:


private:
    QString m_lineEditNetworkSettingsDns1Text;
    QString m_lineEditNetworkSettingsDns2Text;

    QString m_ipAddressValidatorRegex;
};
#endif // NETWORK_SETTINGS_LOGIC_H
