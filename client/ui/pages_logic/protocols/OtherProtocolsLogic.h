#ifndef OTHER_PROTOCOLS_LOGIC_H
#define OTHER_PROTOCOLS_LOGIC_H

#include "PageProtocolLogicBase.h"

class UiLogic;

class OtherProtocolsLogic : public PageProtocolLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(QString, labelTftpUserNameText)
    AUTO_PROPERTY(QString, labelTftpPasswordText)
    AUTO_PROPERTY(QString, labelTftpPortText)

public:
    Q_INVOKABLE void onPushButtonProtoShadowSocksSaveClicked();

public:
    explicit OtherProtocolsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~OtherProtocolsLogic() = default;

    void updateProtocolPage(const QJsonObject &config, DockerContainer container, bool haveAuthData) override;
    //QJsonObject getProtocolConfigFromPage(QJsonObject oldConfig) override;

private:
    Settings m_settings;
    UiLogic *m_uiLogic;

};
#endif // OTHER_PROTOCOLS_LOGIC_H
