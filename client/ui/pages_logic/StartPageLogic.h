#ifndef START_PAGE_LOGIC_H
#define START_PAGE_LOGIC_H

#include "PageLogicBase.h"

#include <QRegularExpression>

class UiLogic;

class StartPageLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pushButtonConnectEnabled)
    AUTO_PROPERTY(bool, pushButtonConnectKeyChecked)
    AUTO_PROPERTY(QString, pushButtonConnectText)
    AUTO_PROPERTY(QString, lineEditStartExistingCodeText)
    AUTO_PROPERTY(QString, textEditSshKeyText)
    AUTO_PROPERTY(QString, lineEditIpText)
    AUTO_PROPERTY(QString, lineEditPasswordText)
    AUTO_PROPERTY(QString, lineEditLoginText)
    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)
    AUTO_PROPERTY(bool, pushButtonBackFromStartVisible)

    AUTO_PROPERTY(QString, privateKeyPassphrase);

    READONLY_PROPERTY(QRegularExpression, ipAddressPortRegex)
public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonConnect();
    Q_INVOKABLE void onPushButtonImport();
    Q_INVOKABLE void onPushButtonImportOpenFile();

#ifdef Q_OS_ANDROID
    Q_INVOKABLE void startQrDecoder();
#endif

    void importAnyFile(const QString &configData);

    bool importConnection(const QJsonObject &profile);
    bool importConnectionFromCode(QString code);
    bool importConnectionFromQr(const QByteArray &data);
    bool importConnectionFromOpenVpnConfig(const QString &config);
    bool importConnectionFromWireguardConfig(const QString &config);

public:
    explicit StartPageLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~StartPageLogic() = default;

signals:
    void showPassphraseRequestMessage();
    void passphraseDialogClosed();
};
#endif // START_PAGE_LOGIC_H
