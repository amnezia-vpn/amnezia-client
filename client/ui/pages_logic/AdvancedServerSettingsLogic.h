#ifndef ADVANCEDSERVERSETTINGSLOGIC_H
#define ADVANCEDSERVERSETTINGSLOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class AdvancedServerSettingsLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)

    AUTO_PROPERTY(QString, pushButtonClearText)
    AUTO_PROPERTY(bool, pushButtonClearVisible)

    AUTO_PROPERTY(QString, labelServerText)
    AUTO_PROPERTY(QString, labelCurrentVpnProtocolText)

public:
    explicit AdvancedServerSettingsLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~AdvancedServerSettingsLogic() = default;

    Q_INVOKABLE void onUpdatePage() override;

    Q_INVOKABLE void onPushButtonClearServer();
};

#endif // ADVANCEDSERVERSETTINGSLOGIC_H
