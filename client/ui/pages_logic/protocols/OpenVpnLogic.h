#ifndef OPENVPN_LOGIC_H
#define OPENVPN_LOGIC_H

#include "../PageLogicBase.h"

class UiLogic;

class OpenVpnLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, lineEditProtoOpenvpnSubnetText)
    AUTO_PROPERTY(bool, radioButtonProtoOpenvpnUdpChecked)
    AUTO_PROPERTY(bool, checkBoxProtoOpenvpnAutoEncryptionChecked)
    AUTO_PROPERTY(QString, comboBoxProtoOpenvpnCipherText)
    AUTO_PROPERTY(QString, comboBoxProtoOpenvpnHashText)
    AUTO_PROPERTY(bool, checkBoxProtoOpenvpnBlockDnsChecked)
    AUTO_PROPERTY(QString, lineEditProtoOpenvpnPortText)
    AUTO_PROPERTY(bool, checkBoxProtoOpenvpnTlsAuthChecked)

    AUTO_PROPERTY(bool, widgetProtoOpenvpnEnabled)
    AUTO_PROPERTY(bool, pushButtonOpenvpnSaveVisible)
    AUTO_PROPERTY(bool, progressBarProtoOpenvpnResetVisible)
    AUTO_PROPERTY(bool, radioButtonProtoOpenvpnUdpEnabled)
    AUTO_PROPERTY(bool, radioButtonProtoOpenvpnTcpEnabled)
    AUTO_PROPERTY(bool, radioButtonProtoOpenvpnTcpChecked)
    AUTO_PROPERTY(bool, lineEditProtoOpenvpnPortEnabled)

    AUTO_PROPERTY(bool, comboBoxProtoOpenvpnCipherEnabled)
    AUTO_PROPERTY(bool, comboBoxProtoOpenvpnHashEnabled)
    AUTO_PROPERTY(bool, pageProtoOpenvpnEnabled)
    AUTO_PROPERTY(bool, labelProtoOpenvpnInfoVisible)
    AUTO_PROPERTY(QString, labelProtoOpenvpnInfoText)
    AUTO_PROPERTY(int, progressBarProtoOpenvpnResetValue)
    AUTO_PROPERTY(int, progressBarProtoOpenvpnResetMaximium)

public:
    Q_INVOKABLE void onCheckBoxProtoOpenvpnAutoEncryptionClicked();
    Q_INVOKABLE void onPushButtonProtoOpenvpnSaveClicked();

public:
    explicit OpenVpnLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OpenVpnLogic() = default;

    void updateOpenVpnPage(const QJsonObject &openvpnConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getOpenVpnConfigFromPage(QJsonObject oldConfig);

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // OPENVPN_LOGIC_H
