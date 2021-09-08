#ifndef SHADOWSOCKS_LOGIC_H
#define SHADOWSOCKS_LOGIC_H

#include "../PageLogicBase.h"

class UiLogic;

class ShadowSocksLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, widgetProtoSsEnabled)
    AUTO_PROPERTY(QString, comboBoxProtoShadowsocksCipherText)
    AUTO_PROPERTY(QString, lineEditProtoShadowsocksPortText)
    AUTO_PROPERTY(bool, pushButtonShadowsocksSaveVisible)
    AUTO_PROPERTY(bool, progressBarProtoShadowsocksResetVisible)
    AUTO_PROPERTY(bool, lineEditProtoShadowsocksPortEnabled)
    AUTO_PROPERTY(bool, pageProtoShadowsocksEnabled)
    AUTO_PROPERTY(bool, labelProtoShadowsocksInfoVisible)
    AUTO_PROPERTY(QString, labelProtoShadowsocksInfoText)
    AUTO_PROPERTY(int, progressBarProtoShadowsocksResetValue)
    AUTO_PROPERTY(int, progressBarProtoShadowsocksResetMaximium)

public:
    Q_INVOKABLE void onPushButtonProtoShadowsocksSaveClicked();

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
