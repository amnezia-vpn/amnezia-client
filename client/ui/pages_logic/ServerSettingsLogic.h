#ifndef SERVER_SETTINGS_LOGIC_H
#define SERVER_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerSettingsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pageServerSettingsEnabled)
    AUTO_PROPERTY(bool, labelServerSettingsWaitInfoVisible)
    AUTO_PROPERTY(QString, labelServerSettingsWaitInfoText)
    AUTO_PROPERTY(QString, pushButtonServerSettingsClearText)
    AUTO_PROPERTY(QString, pushButtonServerSettingsClearClientCacheText)
    AUTO_PROPERTY(bool, pushButtonServerSettingsClearVisible)
    AUTO_PROPERTY(bool, pushButtonServerSettingsClearClientCacheVisible)
    AUTO_PROPERTY(bool, pushButtonServerSettingsShareFullVisible)
    AUTO_PROPERTY(QString, labelServerSettingsServerText)
    AUTO_PROPERTY(QString, lineEditServerSettingsDescriptionText)
    AUTO_PROPERTY(QString, labelServerSettingsCurrentVpnProtocolText)

public:
    Q_INVOKABLE void updateServerSettingsPage();

    Q_INVOKABLE void onPushButtonServerSettingsClearServer();
    Q_INVOKABLE void onPushButtonServerSettingsForgetServer();
    Q_INVOKABLE void onPushButtonServerSettingsShareFullClicked();
    Q_INVOKABLE void onPushButtonServerSettingsClearClientCacheClicked();
    Q_INVOKABLE void onLineEditServerSettingsDescriptionEditingFinished();

public:
    explicit ServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerSettingsLogic() = default;

};
#endif // SERVER_SETTINGS_LOGIC_H
