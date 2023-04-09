#include "ServerConfiguringProgressLogic.h"
#include "defines.h"
#include "core/errorstrings.h"
#include <QTimer>
#include <QEventLoop>

#include "core/servercontroller.h"

ServerConfiguringProgressLogic::ServerConfiguringProgressLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarValue{0},
    m_labelWaitInfoVisible{true},
    m_labelWaitInfoText{tr("Please wait, configuring process may take up to 5 minutes")},
    m_progressBarVisible{true},
    m_progressBarMaximum{100},
    m_progressBarTextVisible{true},
    m_progressBarText{tr("Configuring...")},
    m_labelServerBusyVisible{false},
    m_labelServerBusyText{""}
{

}

void ServerConfiguringProgressLogic::onUpdatePage()
{
    set_progressBarValue(0);
}


ErrorCode ServerConfiguringProgressLogic::doInstallAction(const std::function<ErrorCode()> &action)
{
    PageFunc page;
    page.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    ButtonFunc noButton;
    LabelFunc noWaitInfo;
    ProgressFunc progress;
    progress.setVisibleFunc = [this] (bool visible) -> void {
        set_progressBarVisible(visible);
    };

    progress.setValueFunc = [this] (int value) -> void {
        set_progressBarValue(value);
    };
    progress.getValueFunc = [this] (void) -> int {
        return progressBarValue();
    };
    progress.getMaximumFunc = [this] (void) -> int {
        return progressBarMaximum();
    };

    LabelFunc busyInfo;
    busyInfo.setTextFunc = [this] (const QString& text) -> void {
        set_labelServerBusyText(text);
    };
    busyInfo.setVisibleFunc = [this] (bool visible) -> void {
        set_labelServerBusyVisible(visible);
    };

    ButtonFunc cancelButton;
    cancelButton.setVisibleFunc = [this] (bool visible) -> void {
        set_pushButtonCancelVisible(visible);
    };

    return doInstallAction(action, page, progress, noButton, noWaitInfo, busyInfo, cancelButton);
}

ErrorCode ServerConfiguringProgressLogic::doInstallAction(const std::function<ErrorCode()> &action,
                                                          const PageFunc &page,
                                                          const ProgressFunc &progress,
                                                          const ButtonFunc &saveButton,
                                                          const LabelFunc &waitInfo,
                                                          const LabelFunc &serverBusyInfo,
                                                          const ButtonFunc &cancelButton)
{
    progress.setVisibleFunc(true);
    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (saveButton.setVisibleFunc) {
        saveButton.setVisibleFunc(false);
    }
    if (waitInfo.setVisibleFunc) {
        waitInfo.setVisibleFunc(true);
    }
    if (waitInfo.setTextFunc) {
        waitInfo.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress.setValueFunc(progress.getValueFunc() + 1);
    });

    progress.setValueFunc(0);
    timer.start(1000);

    QMetaObject::Connection cancelDoInstallActionConnection;
    if (cancelButton.setVisibleFunc) {
        cancelDoInstallActionConnection = connect(this, &ServerConfiguringProgressLogic::cancelDoInstallAction,
                m_serverController.get(), &ServerController::setCancelInstallation);
    }


    QMetaObject::Connection serverBusyConnection;
    if (serverBusyInfo.setVisibleFunc && serverBusyInfo.setTextFunc) {
        serverBusyConnection = connect(m_serverController.get(),
                             &ServerController::serverIsBusy,
                             this,
                             [&serverBusyInfo, &timer, &cancelButton](const bool isBusy) {
            isBusy ? timer.stop() : timer.start(1000);
            serverBusyInfo.setVisibleFunc(isBusy);
            serverBusyInfo.setTextFunc(isBusy ? "Amnesia has detected that your server is currently "
                                                "busy installing other software. Amnesia installation "
                                                "will pause until the server finishes installing other software"
                                              : "");
            if (cancelButton.setVisibleFunc) {
                cancelButton.setVisibleFunc(isBusy ? true : false);
            }
        });
    }

    ErrorCode e = action();
    qDebug() << "doInstallAction finished with code" << e;
    if (cancelButton.setVisibleFunc) {
        disconnect(cancelDoInstallActionConnection);
    }

    if (serverBusyInfo.setVisibleFunc && serverBusyInfo.setTextFunc) {
        disconnect(serverBusyConnection);
    }

    if (e) {
        if (page.setEnabledFunc) {
            page.setEnabledFunc(true);
        }
        if (saveButton.setVisibleFunc) {
            saveButton.setVisibleFunc(true);
        }
        if (waitInfo.setVisibleFunc) {
            waitInfo.setVisibleFunc(false);
        }

        progress.setVisibleFunc(false);
        return e;
    }

    // just ui progressbar tweak
    timer.stop();

    int remainingVal = progress.getMaximumFunc() - progress.getValueFunc();

    if (remainingVal > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            progress.setValueFunc(progress.getValueFunc() + 1);
            if (progress.getValueFunc() >= progress.getMaximumFunc()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }


    progress.setVisibleFunc(false);
    if (saveButton.setVisibleFunc) {
        saveButton.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (waitInfo.setTextFunc) {
        waitInfo.setTextFunc(tr("Operation finished"));
    }
    return ErrorCode::NoError;
}

void ServerConfiguringProgressLogic::onPushButtonCancelClicked()
{
    cancelDoInstallAction(true);
}
