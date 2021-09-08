#ifndef SERVER_SETTINGS_LOGIC_H
#define SERVER_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerSettingsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)
    AUTO_PROPERTY(QString, pushButtonClearText)
    AUTO_PROPERTY(QString, pushButtonClearClientCacheText)
    AUTO_PROPERTY(bool, pushButtonClearVisible)
    AUTO_PROPERTY(bool, pushButtonClearClientCacheVisible)
    AUTO_PROPERTY(bool, pushButtonShareFullVisible)
    AUTO_PROPERTY(QString, labelServerText)
    AUTO_PROPERTY(QString, lineEditDescriptionText)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)

public:
    Q_INVOKABLE void updatePage() override;

    Q_INVOKABLE void onPushButtonClearServer();
    Q_INVOKABLE void onPushButtonForgetServer();
    Q_INVOKABLE void onPushButtonShareFullClicked();
    Q_INVOKABLE void onPushButtonClearClientCacheClicked();
    Q_INVOKABLE void onLineEditDescriptionEditingFinished();

public:
    explicit ServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerSettingsLogic() = default;

};
#endif // SERVER_SETTINGS_LOGIC_H
