#ifndef SHARE_CONNECTION_LOGIC_H
#define SHARE_CONNECTION_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ShareConnectionLogic: public PageLogicBase
{
    Q_OBJECT

public:
    AUTO_PROPERTY(bool, shareFullAccess)

    AUTO_PROPERTY(QString, textEditShareAmneziaCodeText)
    AUTO_PROPERTY(QStringList, shareAmneziaQrCodeTextSeries)
    AUTO_PROPERTY(int, shareAmneziaQrCodeTextSeriesLength)

    AUTO_PROPERTY(QString, textEditShareOpenVpnCodeText)

    AUTO_PROPERTY(QString, textEditShareShadowSocksText)
    AUTO_PROPERTY(QString, lineEditShareShadowSocksStringText)
    AUTO_PROPERTY(QString, shareShadowSocksQrCodeText)

    AUTO_PROPERTY(QString, textEditShareCloakText)

    AUTO_PROPERTY(QString, textEditShareWireGuardCodeText)
    AUTO_PROPERTY(QString, shareWireGuardQrCodeText)

    AUTO_PROPERTY(QString, textEditShareIkev2CertText)
    AUTO_PROPERTY(QString, textEditShareIkev2MobileConfigText)
    AUTO_PROPERTY(QString, textEditShareIkev2StrongSwanConfigText)

public:
    Q_INVOKABLE void onPushButtonShareAmneziaGenerateClicked();
    Q_INVOKABLE void onPushButtonShareOpenVpnGenerateClicked();
    Q_INVOKABLE void onPushButtonShareShadowSocksGenerateClicked();
    Q_INVOKABLE void onPushButtonShareCloakGenerateClicked();
    Q_INVOKABLE void onPushButtonShareWireGuardGenerateClicked();
    Q_INVOKABLE void onPushButtonShareIkev2GenerateClicked();

    Q_INVOKABLE virtual void onUpdatePage() override;

public:
    explicit ShareConnectionLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ShareConnectionLogic() = default;

    void updateSharingPage(int serverIndex, DockerContainer container);
    QList<QString> genQrCodeImageSeries(const QByteArray &data);

    QString imageToBase64(const QImage &image);


};
#endif // SHARE_CONNECTION_LOGIC_H
