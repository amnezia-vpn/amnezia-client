#ifndef NEW_SERVER_CONFIGURING_LOGIC_H
#define NEW_SERVER_CONFIGURING_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class NewServerConfiguringLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(double, progressBarValue)
    AUTO_PROPERTY(bool, pageEnabled)
    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)
    AUTO_PROPERTY(bool, progressBarVisible)
    AUTO_PROPERTY(int, progressBarMaximium)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)

public:
    explicit NewServerConfiguringLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~NewServerConfiguringLogic() = default;

};
#endif // NEW_SERVER_CONFIGURING_LOGIC_H
