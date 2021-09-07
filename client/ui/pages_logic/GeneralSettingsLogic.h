#ifndef GENERAL_SETTINGS_LOGIC_H
#define GENERAL_SETTINGS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class GeneralSettingsLogic : public PageLogicBase
{
    Q_OBJECT

public:
    Q_INVOKABLE void updateGeneralSettingPage();

    Q_PROPERTY(bool pushButtonGeneralSettingsShareConnectionEnable READ getPushButtonGeneralSettingsShareConnectionEnable WRITE setPushButtonGeneralSettingsShareConnectionEnable NOTIFY pushButtonGeneralSettingsShareConnectionEnableChanged)

    Q_INVOKABLE void onPushButtonGeneralSettingsServerSettingsClicked();
    Q_INVOKABLE void onPushButtonGeneralSettingsShareConnectionClicked();

public:
    explicit GeneralSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~GeneralSettingsLogic() = default;



    bool getPushButtonGeneralSettingsShareConnectionEnable() const;
    void setPushButtonGeneralSettingsShareConnectionEnable(bool pushButtonGeneralSettingsShareConnectionEnable);

signals:
    void pushButtonGeneralSettingsShareConnectionEnableChanged();


private:


private slots:


private:
    bool m_pushButtonGeneralSettingsShareConnectionEnable;

};
#endif // GENERAL_SETTINGS_LOGIC_H
