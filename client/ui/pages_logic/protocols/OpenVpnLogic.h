#ifndef OPENVPN_LOGIC_H
#define OPENVPN_LOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class OpenVpnLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditSubnetText)

    AUTO_PROPERTY(bool, radioButtonTcpEnabled)
    AUTO_PROPERTY(bool, radioButtonUdpEnabled)
    AUTO_PROPERTY(bool, radioButtonTcpChecked)
    AUTO_PROPERTY(bool, radioButtonUdpChecked)

    AUTO_PROPERTY(bool, checkBoxAutoEncryptionChecked)
    AUTO_PROPERTY(QString, comboBoxVpnCipherText)
    AUTO_PROPERTY(QString, comboBoxVpnHashText)
    AUTO_PROPERTY(bool, checkBoxBlockDnsChecked)
    AUTO_PROPERTY(QString, lineEditPortText)
    AUTO_PROPERTY(bool, checkBoxTlsAuthChecked)
    AUTO_PROPERTY(QString, textAreaAdditionalClientConfig)
    AUTO_PROPERTY(QString, textAreaAdditionalServerConfig)

    AUTO_PROPERTY(bool, pushButtonSaveVisible)
    AUTO_PROPERTY(bool, progressBarResetVisible)

    AUTO_PROPERTY(bool, lineEditPortEnabled)

    AUTO_PROPERTY(bool, labelProtoOpenVpnInfoVisible)
    AUTO_PROPERTY(QString, labelProtoOpenVpnInfoText)
    AUTO_PROPERTY(int, progressBarResetValue)
    AUTO_PROPERTY(int, progressBarResetMaximum)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)

    AUTO_PROPERTY(bool, labelServerBusyVisible)
    AUTO_PROPERTY(QString, labelServerBusyText)

    AUTO_PROPERTY(bool, pushButtonCancelVisible)

    AUTO_PROPERTY(QString, openVpnLastConfigText)
    AUTO_PROPERTY(bool, isThirdPartyConfig)

public:
    Q_INVOKABLE void onPushButtonSaveClicked();
    Q_INVOKABLE void onPushButtonCancelClicked();

public:
    explicit OpenVpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OpenVpnLogic() = default;

    void updateProtocolPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData) override;
    QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    UiLogic *m_uiLogic;

};
#endif // OPENVPN_LOGIC_H
