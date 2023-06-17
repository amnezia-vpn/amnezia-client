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
    AUTO_PROPERTY(int, progressBarMaximum)
    AUTO_PROPERTY(bool, progressBarTextVisible)
    AUTO_PROPERTY(QString, progressBarText)
    AUTO_PROPERTY(bool, labelServerBusyVisible)
    AUTO_PROPERTY(QString, labelServerBusyText)
    AUTO_PROPERTY(bool, pushButtonCancelVisible)

public:
    Q_INVOKABLE void onPushButtonCancelClicked();

private:
    struct ProgressFunc {
        std::function<void(bool)> setVisibleFunc;
        std::function<void(int)> setValueFunc;
        std::function<int(void)> getValueFunc;
        std::function<int(void)> getMaximumFunc;
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

public:
    explicit ServerConfiguringProgressLogic(UiLogic *uiLogic, QObject *parent = nullptr);
    ~ServerConfiguringProgressLogic() = default;

    friend class OpenVpnLogic;
    friend class ShadowSocksLogic;
    friend class CloakLogic;
    friend class UiLogic;

    void onUpdatePage() override;
    ErrorCode doInstallAction(const std::function<ErrorCode()> &action);
    ErrorCode doInstallAction(const std::function<ErrorCode()> &action,
                              const PageFunc &page,
                              const ProgressFunc &progress,
                              const ButtonFunc &saveButton,
                              const LabelFunc &waitInfo,
                              const LabelFunc &serverBusyInfo,
                              const ButtonFunc &cancelButton);

signals:
    void cancelDoInstallAction(const bool cancel);

};
#endif // SERVER_CONFIGURING_PROGRESS_LOGIC_H
