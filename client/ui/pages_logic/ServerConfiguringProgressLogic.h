#ifndef SERVER_CONFIGURING_PROGRESS_LOGIC_H
#define SERVER_CONFIGURING_PROGRESS_LOGIC_H

#include <functional>
#include "PageLogicBase.h"
#include "core/defs.h"

using namespace amnezia;

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

    void onUpdatePage() override;
    ErrorCode doInstallAction(const std::function<ErrorCode()> &action);

private:
    struct ProgressFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(int)> setValueFunc;
        std::function<int(void)> getValueFunc;
        std::function<int(void)> getMaximiumFunc;
        std::function<void(bool)> setTextVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };
    struct PageFunc {
        std::function<void(bool)> setEnabledFunc;
    };
    struct ButtonFunc {
        std::function<void(bool)> setVisibleFunc;
    };
    struct LabelFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(const QString&)> setTextFunc;
    };

};
#endif // SERVER_CONFIGURING_PROGRESS_LOGIC_H
