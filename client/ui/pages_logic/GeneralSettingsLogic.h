#ifndef GENERAL_SETTINGS_LOGIC_H
#define GENERAL_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class GeneralSettingsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, pushButtonGeneralSettingsShareConnectionEnable)

public:
    Q_INVOKABLE void updateGeneralSettingPage();
    Q_INVOKABLE void onPushButtonGeneralSettingsServerSettingsClicked();
    Q_INVOKABLE void onPushButtonGeneralSettingsShareConnectionClicked();

public:
    explicit GeneralSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~GeneralSettingsLogic() = default;

};
#endif // GENERAL_SETTINGS_LOGIC_H
