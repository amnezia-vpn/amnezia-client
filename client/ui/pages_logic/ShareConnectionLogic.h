#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "PageLogicBase.h"
#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

class UiLogic;

class ShareConnectionLogic: public PageLogicBase
{
    Q_OBJECT

public:
    AUTO_PROPERTY(bool, shareFullAccess)

    AUTO_PROPERTY(QString, textEditShareAmneziaCodeText)
    AUTO_PROPERTY(QString, shareAmneziaQrCodeText)

    AUTO_PROPERTY(QString, textEditShareOpenVpnCodeText)
    AUTO_PROPERTY(QString, pushButtonShareOpenVpnGenerateText)

    AUTO_PROPERTY(QString, lineEditShareShadowSocksStringText)
    AUTO_PROPERTY(QString, shareShadowSocksQrCodeText)
    AUTO_PROPERTY(QString, labelShareShadowSocksServerText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPortText)
    AUTO_PROPERTY(QString, labelShareShadowSocksMethodText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPasswordText)

    AUTO_PROPERTY(QString, textEditShareCloakText)

public:
    Q_INVOKABLE void onPushButtonShareAmneziaGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenVpnGenerateClicked();

    Q_INVOKABLE virtual void onUpdatePage() override;

public:
    explicit ShareConnectionLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShareConnectionLogic() = default;

    void updateSharingPage(int serverIndex, const ServerCredentials &credentials,
                           DockerContainer container);
    QImage updateQRCodeImage(const QString &text);
    QString imageToBase64(const QImage &image);

private:
    CQR_Encode m_qrEncode;

};
#endif // SHARE_CONNECTION_LOGIC_H
