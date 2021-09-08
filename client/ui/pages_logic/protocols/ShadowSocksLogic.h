#ifndef SHADOWSOCKS_LOGIC_H
#define SHADOWSOCKS_LOGIC_H

#include "../PageLogicBase.h"

class UiLogic;

class ShadowSocksLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, widgetProtoShadowSocksEnabled)
    AUTO_PROPERTY(QString, comboBoxProtoShadowSocksCipherText)
    AUTO_PROPERTY(QString, lineEditProtoShadowSocksPortText)
    AUTO_PROPERTY(bool, pushButtonShadowSocksSaveVisible)
    AUTO_PROPERTY(bool, progressBarProtoShadowSocksResetVisible)
    AUTO_PROPERTY(bool, lineEditProtoShadowSocksPortEnabled)
    AUTO_PROPERTY(bool, pageProtoShadowSocksEnabled)
    AUTO_PROPERTY(bool, labelProtoShadowSocksInfoVisible)
    AUTO_PROPERTY(QString, labelProtoShadowSocksInfoText)
    AUTO_PROPERTY(int, progressBarProtoShadowSocksResetValue)
    AUTO_PROPERTY(int, progressBarProtoShadowSocksResetMaximium)

public:
    Q_INVOKABLE void onPushButtonProtoShadowSocksSaveClicked();

public:
    explicit ShadowSocksLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShadowSocksLogic() = default;

    void updateShadowSocksPage(const QJsonObject &ssConfig, DockerContainer container, bool haveAuthData);
    QJsonObject getShadowSocksConfigFromPage(QJsonObject oldConfig);

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // SHADOWSOCKS_LOGIC_H
