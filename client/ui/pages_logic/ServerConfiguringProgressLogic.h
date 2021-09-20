#ifndef SERVER_CONFIGURING_PROGRESS_LOGIC_H
#define SERVER_CONFIGURING_PROGRESS_LOGIC_H

#include "PageLogicBase.h"

class UiLogic;

class ServerConfiguringProgressLogic : public PageLogicBase
{
    Q_OBJECT

    AUTO_PROPERTY(double, progressBarValue)
    AUTO_PROPERTY(bool, labelWaitInfoVisible)
    AUTO_PROPERTY(QString, labelWaitInfoText)
    AUTO_PROPERTY(bool, progressBarVisible)
    AUTO_PROPERTY(int, progressBarMaximium)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)

public:
    explicit ServerConfiguringProgressLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerConfiguringProgressLogic() = default;

};
#endif // SERVER_CONFIGURING_PROGRESS_LOGIC_H
