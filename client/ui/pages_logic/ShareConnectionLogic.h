#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "PageLogicBase.h"
#include "3rd/QRCodeGenerator/QRCodeGenerator.h"

class UiLogic;

class ShareConnectionLogic: public PageLogicBase
{
    Q_OBJECT

public:
    AUTO_PROPERTY(QString, textEditShareOpenVpnCodeText)
    AUTO_PROPERTY(bool, pushButtonShareOpenVpnCopyEnabled)
    AUTO_PROPERTY(bool, pushButtonShareOpenVpnSaveEnabled)
    AUTO_PROPERTY(QString, lineEditShareShadowSocksStringText)
    AUTO_PROPERTY(QString, labelShareShadowSocksQrCodeText)
    AUTO_PROPERTY(QString, labelShareShadowSocksServerText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPortText)
    AUTO_PROPERTY(QString, labelShareShadowSocksMethodText)
    AUTO_PROPERTY(QString, labelShareShadowSocksPasswordText)
    AUTO_PROPERTY(QString, plainTextEditShareCloakText)
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
    QImage updateQRCodeImage(const QString &text);
    QString imageToBase64(const QImage &image);

private:
    CQR_Encode m_qrEncode;

};
#endif // SHARE_CONNECTION_LOGIC_H
