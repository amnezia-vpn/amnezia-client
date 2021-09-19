#ifndef SERVER_CONTAINERS_LOGIC_H
#define SERVER_CONTAINERS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerContainersLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(int, progressBarProtocolsContainerReinstallValue)
    AUTO_PROPERTY(int, progressBarProtocolsContainerReinstallMaximium)

//    AUTO_PROPERTY(bool, pushButtonOpenVpnContInstallChecked)
//    AUTO_PROPERTY(bool, pushButtonSsOpenVpnContInstallChecked)
//    AUTO_PROPERTY(bool, pushButtonCloakOpenVpnContInstallChecked)
//    AUTO_PROPERTY(bool, pushButtonWireguardContInstallChecked)

    AUTO_PROPERTY(bool, pushButtonOpenVpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonSsOpenVpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonCloakOpenVpnContInstallEnabled)
    AUTO_PROPERTY(bool, pushButtonWireguardContInstallEnabled)

    AUTO_PROPERTY(bool, pushButtonOpenVpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonSsOpenVpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonCloakOpenVpnContDefaultChecked)
    AUTO_PROPERTY(bool, pushButtonWireguardContDefaultChecked)

    AUTO_PROPERTY(bool, pushButtonOpenVpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonSsOpenVpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonCloakOpenVpnContDefaultVisible)
    AUTO_PROPERTY(bool, pushButtonWireguardContDefaultVisible)

    AUTO_PROPERTY(bool, pushButtonOpenVpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonSsOpenVpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonCloakOpenVpnContShareVisible)
    AUTO_PROPERTY(bool, pushButtonWireguardContShareVisible)

    AUTO_PROPERTY(bool, frameOpenvpnSettingsVisible)
    AUTO_PROPERTY(bool, frameOpenvpnSsSettingsVisible)
    AUTO_PROPERTY(bool, frameOpenvpnSsCloakSettingsVisible)
    AUTO_PROPERTY(bool, progressBarProtocolsContainerReinstallVisible)

    AUTO_PROPERTY(bool, frameWireguardSettingsVisible)
    AUTO_PROPERTY(bool, frameWireguardVisible)

public:
    Q_INVOKABLE void updateServerContainersPage();
    Q_INVOKABLE void onPushButtonProtoSettingsClicked(DockerContainer c, Protocol p);
    Q_INVOKABLE void onPushButtonDefaultClicked(DockerContainer c);
    Q_INVOKABLE void onPushButtonShareClicked(DockerContainer c);

public:
    explicit ServerContainersLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerContainersLogic() = default;

    void setupProtocolsPageConnections();

signals:
    void pushButtonOpenVpnContDefaultClicked(bool checked);
    void pushButtonSsOpenVpnContDefaultClicked(bool checked);
    void pushButtonCloakOpenVpnContDefaultClicked(bool checked);
    void pushButtonWireguardContDefaultClicked(bool checked);
    void pushButtonOpenVpnContInstallClicked(bool checked);
    void pushButtonSsOpenVpnContInstallClicked(bool checked);
    void pushButtonCloakOpenVpnContInstallClicked(bool checked);
    void pushButtonWireguardContInstallClicked(bool checked);
    void pushButtonOpenVpnContShareClicked(bool checked);
    void pushButtonSsOpenVpnContShareClicked(bool checked);
    void pushButtonCloakOpenVpnContShareClicked(bool checked);
    void pushButtonWireguardContShareClicked(bool checked);

};
#endif // SERVER_CONTAINERS_LOGIC_H
