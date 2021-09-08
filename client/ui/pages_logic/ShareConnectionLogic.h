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
    AUTO_PROPERTY(bool, pageShareOpenvpnVisible)
    AUTO_PROPERTY(bool, pageShareShadowsocksVisible)
    AUTO_PROPERTY(bool, pageShareCloakVisible)
    AUTO_PROPERTY(bool, pageShareFullAccessVisible)
    AUTO_PROPERTY(QString, textEditShareOpenvpnCodeText)
    AUTO_PROPERTY(bool, pushButtonShareOpenvpnCopyEnabled)
    AUTO_PROPERTY(bool, pushButtonShareOpenvpnSaveEnabled)
    AUTO_PROPERTY(int, toolBoxShareConnectionCurrentIndex)
    AUTO_PROPERTY(bool, pushButtonShareSsCopyEnabled)
    AUTO_PROPERTY(QString, lineEditShareSsStringText)
    AUTO_PROPERTY(QString, labelShareSsQrCodeText)
    AUTO_PROPERTY(QString, labelShareSsServerText)
    AUTO_PROPERTY(QString, labelShareSsPortText)
    AUTO_PROPERTY(QString, labelShareSsMethodText)
    AUTO_PROPERTY(QString, labelShareSsPasswordText)
    AUTO_PROPERTY(QString, plainTextEditShareCloakText)
    AUTO_PROPERTY(bool, pushButtonShareCloakCopyEnabled)
    AUTO_PROPERTY(QString, textEditShareFullCodeText)
    AUTO_PROPERTY(QString, textEditShareAmneziaCodeText)
    AUTO_PROPERTY(QString, pushButtonShareFullCopyText)
    AUTO_PROPERTY(QString, pushButtonShareAmneziaCopyText)
    AUTO_PROPERTY(QString, pushButtonShareOpenvpnCopyText)
    AUTO_PROPERTY(QString, pushButtonShareSsCopyText)
    AUTO_PROPERTY(QString, pushButtonShareCloakCopyText)
    AUTO_PROPERTY(bool, pushButtonShareAmneziaGenerateEnabled)
    AUTO_PROPERTY(bool, pushButtonShareAmneziaCopyEnabled)
    AUTO_PROPERTY(QString, pushButtonShareAmneziaGenerateText)
    AUTO_PROPERTY(bool, pushButtonShareOpenvpnGenerateEnabled)
    AUTO_PROPERTY(QString, pushButtonShareOpenvpnGenerateText)

public:
    Q_INVOKABLE void onPushButtonShareFullCopyClicked();
    Q_INVOKABLE void onPushButtonShareFullSaveClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaSaveClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnCopyClicked();
    Q_INVOKABLE void onPushButtonShareSsCopyClicked();
    Q_INVOKABLE void onPushButtonShareCloakCopyClicked();
    Q_INVOKABLE void onPushButtonShareAmneziaGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenvpnSaveClicked();

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
