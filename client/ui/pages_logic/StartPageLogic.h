#ifndef START_PAGE_LOGIC_H
#define START_PAGE_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class StartPageLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pushButtonNewServerConnectEnabled)
    AUTO_PROPERTY(bool, pushButtonNewServerConnectKeyChecked)
    AUTO_PROPERTY(QString, pushButtonNewServerConnectText)
    AUTO_PROPERTY(QString, lineEditStartExistingCodeText)
    AUTO_PROPERTY(QString, textEditNewServerSshKeyText)
    AUTO_PROPERTY(QString, lineEditNewServerIpText)
    AUTO_PROPERTY(QString, lineEditNewServerPasswordText)
    AUTO_PROPERTY(QString, lineEditNewServerLoginText)
    AUTO_PROPERTY(bool, labelNewServerWaitInfoVisible)
    AUTO_PROPERTY(QString, labelNewServerWaitInfoText)
    AUTO_PROPERTY(bool, pushButtonBackFromStartVisible)
    AUTO_PROPERTY(bool, pushButtonNewServerConnectVisible)

public:
    Q_INVOKABLE void updateStartPage();

    Q_INVOKABLE void onPushButtonNewServerConnect();
    Q_INVOKABLE void onPushButtonNewServerImport();

public:
    explicit StartPageLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~StartPageLogic() = default;

    QString getPushButtonNewServerConnectText() const;
    void setPushButtonNewServerConnectText(const QString &pushButtonNewServerConnectText);

};
#endif // START_PAGE_LOGIC_H
