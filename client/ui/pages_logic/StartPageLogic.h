#ifndef START_PAGE_LOGIC_H
#define START_PAGE_LOGIC_H

#include "PageLogicBase.h"
#include "defines.h"

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
    AUTO_PROPERTY(bool, pushButtonConnectVisible)

    READONLY_PROPERTY(QRegExp, ipAddressPortRegex)
public:
    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonConnect();
    Q_INVOKABLE void onPushButtonImport();
    Q_INVOKABLE void onPushButtonImportOpenFile();

    bool importConnection(const QJsonObject &profile);
    bool importConnectionFromCode(QString code);
    bool importConnectionFromQr(const QByteArray &data);

public:
    explicit StartPageLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~StartPageLogic() = default;

};
#endif // START_PAGE_LOGIC_H
