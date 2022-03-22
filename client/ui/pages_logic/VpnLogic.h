#ifndef VPN_LOGIC_H
#define VPN_LOGIC_H

#include "PageLogicBase.h"
#include "protocols/vpnprotocol.h"

class UiLogic;

class VpnLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pushButtonConnectChecked)
    AUTO_PROPERTY(QString, labelSpeedReceivedText)
    AUTO_PROPERTY(QString, labelSpeedSentText)
    AUTO_PROPERTY(QString, labelStateText)
    AUTO_PROPERTY(QString, labelCurrentServer)
    AUTO_PROPERTY(QString, labelCurrentService)
    AUTO_PROPERTY(QString, labelCurrentDns)
    AUTO_PROPERTY(bool, amneziaDnsEnabled)

    AUTO_PROPERTY(bool, pushButtonConnectEnabled)
    AUTO_PROPERTY(bool, pushButtonConnectVisible)
    AUTO_PROPERTY(bool, widgetVpnModeEnabled)
    AUTO_PROPERTY(bool, isContainerSupportedByCurrentPlatform)
    AUTO_PROPERTY(bool, isContainerHaveAuthData)

    AUTO_PROPERTY(QString, labelErrorText)
    AUTO_PROPERTY(QString, labelVersionText)

    AUTO_PROPERTY(bool, isCustomRoutesSupported)

    AUTO_PROPERTY(bool, radioButtonVpnModeAllSitesChecked)
    AUTO_PROPERTY(bool, radioButtonVpnModeForwardSitesChecked)
    AUTO_PROPERTY(bool, radioButtonVpnModeExceptSitesChecked)

public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onRadioButtonVpnModeAllSitesClicked();
    Q_INVOKABLE void onRadioButtonVpnModeForwardSitesClicked();
    Q_INVOKABLE void onRadioButtonVpnModeExceptSitesClicked();

    Q_INVOKABLE void onPushButtonConnectClicked();

public:
    explicit VpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~VpnLogic() = default;

    bool getPushButtonConnectChecked() const;
    void setPushButtonConnectChecked(bool pushButtonConnectChecked);

public slots:
    void onConnect();
    void onConnectWorker(int serverIndex, const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);
    void onDisconnect();

    void onBytesChanged(quint64 receivedBytes, quint64 sentBytes);
    void onConnectionStateChanged(VpnProtocol::VpnConnectionState state);
    void onVpnProtocolError(amnezia::ErrorCode errorCode);

signals:
    void connectToVpn(int serverIndex,
        const ServerCredentials &credentials, DockerContainer container, const QJsonObject &containerConfig);

    void disconnectFromVpn();
};
#endif // VPN_LOGIC_H
