#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "PageLogicBase.h"
#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

class UiLogic;

class ShareConnectionLogic: public PageLogicBase
{
    Q_OBJECT

public:
    AUTO_PROPERTY(bool, pageShareAmneziaVisible)
    AUTO_PROPERTY(bool, pageShareOpenVpnVisible)
    AUTO_PROPERTY(bool, pageShareShadowSocksVisible)
    AUTO_PROPERTY(bool, pageShareCloakVisible)
    AUTO_PROPERTY(bool, pageShareFullAccessVisible)
    AUTO_PROPERTY(QString, textEditShareOpenVpnCodeText)
    AUTO_PROPERTY(bool, pushButtonShareOpenVpnCopyEnabled)
    AUTO_PROPERTY(bool, pushButtonShareOpenVpnSaveEnabled)
    AUTO_PROPERTY(int, toolBoxShareConnectionCurrentIndex)
    AUTO_PROPERTY(bool, pushButtonShareShadowSocksCopyEnabled)
    AUTO_PROPERTY(QString, lineEditShareShadowSocksStringText)
    AUTO_PROPERTY(QString, labelShareShadowSocksQrCodeText)
    AUTO_PROPERTY(QString, labelShareShadowSocksServerText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPortText)
    AUTO_PROPERTY(QString, labelShareShadowSocksMethodText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPasswordText)
    AUTO_PROPERTY(QString, plainTextEditShareCloakText)
    AUTO_PROPERTY(bool, pushButtonShareCloakCopyEnabled)
    AUTO_PROPERTY(QString, textEditShareFullCodeText)
    AUTO_PROPERTY(QString, textEditShareAmneziaCodeText)
    AUTO_PROPERTY(QString, pushButtonShareFullCopyText)
    AUTO_PROPERTY(QString, pushButtonShareAmneziaCopyText)
    AUTO_PROPERTY(QString, pushButtonShareOpenVpnCopyText)
    AUTO_PROPERTY(QString, pushButtonShareShadowSocksCopyText)
    AUTO_PROPERTY(QString, pushButtonShareCloakCopyText)
    AUTO_PROPERTY(bool, pushButtonShareAmneziaGenerateEnabled)
    AUTO_PROPERTY(bool, pushButtonShareAmneziaCopyEnabled)
    AUTO_PROPERTY(QString, pushButtonShareAmneziaGenerateText)
    AUTO_PROPERTY(bool, pushButtonShareOpenVpnGenerateEnabled)
    AUTO_PROPERTY(QString, pushButtonShareOpenVpnGenerateText)

public:
    Q_INVOKABLE void onPushButtonShareFullCopyClicked();
    Q_INVOKABLE void onPushButtonShareFullSaveClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaSaveClicked();
    Q_INVOKABLE void onPushButtonShareOpenVpnCopyClicked();
    Q_INVOKABLE void onPushButtonShareShadowSocksCopyClicked();
    Q_INVOKABLE void onPushButtonShareCloakCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenVpnGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenVpnSaveClicked();

public:
    explicit ShareConnectionLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShareConnectionLogic() = default;

    void updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                           DockerContainer container);
    void updateQRCodeImage(const QString &text, const std::function<void(const QString&)>& setLabelFunc);

private:
    CQR_Encode m_qrEncode;

};
#endif // SHARE_CONNECTION_LOGIC_H
