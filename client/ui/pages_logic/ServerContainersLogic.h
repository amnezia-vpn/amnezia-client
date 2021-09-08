#ifndef SERVER_CONTAINERS_LOGIC_H
#define SERVER_CONTAINERS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerContainersLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(int, progressBarProtocolsContainerReinstallValue)
    AUTO_PROPERTY(int, progressBarProtocolsContainerReinstallMaximium)

    AUTO_PROPERTY(bool, pushButtonOpenvpnContInstallChecked)
    AUTO_PROPERTY(bool, pushButtonSsOpenvpnContInstallChecked)
    AUTO_PROPERTY(bool, pushButtonCloakOpenvpnContInstallChecked)
    AUTO_PROPERTY(bool, pushButtonWireguardContInstallChecked)
    AUTO_PROPERTY(bool, pushButtonOpenvpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonSsOpenvpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonCloakOpenvpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonWireguardContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonOpenvpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonSsOpenvpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonCloakOpenvpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonWireguardContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonOpenvpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonSsOpenvpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonCloakOpenvpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonWireguardContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonOpenvpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonSsOpenvpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonCloakOpenvpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonWireguardContShareVisible)
    AUTO_PROPERTY(bool, frameOpenvpnSettingsVisible)
    AUTO_PROPERTY(bool, frameOpenvpnSsSettingsVisible)
    AUTO_PROPERTY(bool, frameOpenvpnSsCloakSettingsVisible)
    AUTO_PROPERTY(bool, progressBarProtocolsContainerReinstallVisible)

    AUTO_PROPERTY(bool, frameWireguardSettingsVisible)
    AUTO_PROPERTY(bool, frameWireguardVisible)

public:
    Q_INVOKABLE void updateServerContainersPage();

    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContSsConfigClicked();
    Q_INVOKABLE void onPushButtonProtoCloakOpenvpnContCloakConfigClicked();
    Q_INVOKABLE void onPushButtonProtoOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContOpenvpnConfigClicked();
    Q_INVOKABLE void onPushButtonProtoSsOpenvpnContSsConfigClicked();

public:
    explicit ServerContainersLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerContainersLogic() = default;

    void setupProtocolsPageConnections();

signals:
    void pushButtonOpenvpnContDefaultClicked(bool checked);
    void pushButtonSsOpenvpnContDefaultClicked(bool checked);
    void pushButtonCloakOpenvpnContDefaultClicked(bool checked);
    void pushButtonWireguardContDefaultClicked(bool checked);
    void pushButtonOpenvpnContInstallClicked(bool checked);
    void pushButtonSsOpenvpnContInstallClicked(bool checked);
    void pushButtonCloakOpenvpnContInstallClicked(bool checked);
    void pushButtonWireguardContInstallClicked(bool checked);
    void pushButtonOpenvpnContShareClicked(bool checked);
    void pushButtonSsOpenvpnContShareClicked(bool checked);
    void pushButtonCloakOpenvpnContShareClicked(bool checked);
    void pushButtonWireguardContShareClicked(bool checked);

};
#endif // SERVER_CONTAINERS_LOGIC_H
