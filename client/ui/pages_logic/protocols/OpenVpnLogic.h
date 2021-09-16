#ifndef OPENVPN_LOGIC_H
#define OPENVPN_LOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class OpenVpnLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditProtoOpenVpnSubnetText)

    AUTO_PROPERTY(bool, radioButtonProtoOpenVpnTcpEnabled)
    AUTO_PROPERTY(bool, radioButtonProtoOpenVpnUdpEnabled)
    AUTO_PROPERTY(bool, radioButtonProtoOpenVpnTcpChecked)
    AUTO_PROPERTY(bool, radioButtonProtoOpenVpnUdpChecked)

    AUTO_PROPERTY(bool, checkBoxProtoOpenVpnAutoEncryptionChecked)
    AUTO_PROPERTY(QString, comboBoxProtoOpenVpnCipherText)
    AUTO_PROPERTY(QString, comboBoxProtoOpenVpnHashText)
    AUTO_PROPERTY(bool, checkBoxProtoOpenVpnBlockDnsChecked)
    AUTO_PROPERTY(QString, lineEditProtoOpenVpnPortText)
    AUTO_PROPERTY(bool, checkBoxProtoOpenVpnTlsAuthChecked)

    AUTO_PROPERTY(bool, widgetProtoOpenVpnEnabled)
    AUTO_PROPERTY(bool, pushButtonOpenvpnSaveVisible)
    AUTO_PROPERTY(bool, progressBarProtoOpenVpnResetVisible)

    AUTO_PROPERTY(bool, lineEditProtoOpenVpnPortEnabled)

    AUTO_PROPERTY(bool, comboBoxProtoOpenVpnCipherEnabled)
    AUTO_PROPERTY(bool, comboBoxProtoOpenVpnHashEnabled)
    AUTO_PROPERTY(bool, labelProtoOpenVpnInfoVisible)
    AUTO_PROPERTY(QString, labelProtoOpenVpnInfoText)
    AUTO_PROPERTY(int, progressBarProtoOpenVpnResetValue)
    AUTO_PROPERTY(int, progressBarProtoOpenVpnResetMaximium)

public:
    Q_INVOKABLE void onCheckBoxProtoOpenVpnAutoEncryptionClicked();
    Q_INVOKABLE void onPushButtonProtoOpenVpnSaveClicked();

public:
    explicit OpenVpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OpenVpnLogic() = default;

    void updateProtocolPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData) override;
    QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // OPENVPN_LOGIC_H
