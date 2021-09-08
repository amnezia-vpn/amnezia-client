#ifndef APP_SETTINGS_LOGIC_H
#define APP_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class AppSettingsLogic : public PageLogicBase
{
    Q_OBJECT
    AUTO_PROPERTY(bool, checkBoxAutostartChecked)
    AUTO_PROPERTY(bool, checkBoxAutoConnectChecked)
    AUTO_PROPERTY(bool, checkBoxStartMinimizedChecked)
    AUTO_PROPERTY(QString, labelVersionText)

public:
    Q_INVOKABLE void updatePage() override;

    Q_INVOKABLE void onCheckBoxAutostartToggled(bool checked);
    Q_INVOKABLE void onCheckBoxAutoconnectToggled(bool checked);
    Q_INVOKABLE void onCheckBoxStartMinimizedToggled(bool checked);
    Q_INVOKABLE void onPushButtonOpenLogsClicked();

public:
    explicit AppSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~AppSettingsLogic() = default;

};
#endif // APP_SETTINGS_LOGIC_H
