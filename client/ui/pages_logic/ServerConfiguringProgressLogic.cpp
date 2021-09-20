#include "ServerConfiguringProgressLogic.h"
#include "defines.h"
#include "core/errorstrings.h"
#include <QTimer>
#include <QEventLoop>
#include <QMessageBox>

ServerConfiguringProgressLogic::ServerConfiguringProgressLogic(UiLogic *logic, QObject *parent):
    PageLogicBase(logic, parent),
    m_progressBarValue{0},
    m_labelWaitInfoVisible{true},
    m_labelWaitInfoText{tr("Please wait, configuring process may take up to 5 minutes")},
    m_progressBarVisible{true},
    m_progressBarMaximium{100},
    m_progressBarTextVisible{true},
    m_progressBarText{tr("Configuring...")}
{

}


ErrorCode ServerConfiguringProgressLogic::doInstallAction(const std::function<ErrorCode()> &action)
{
    PageFunc page;
    page.setEnabledFunc = [this] (bool enabled) -> void {
        set_pageEnabled(enabled);
    };
    ButtonFunc button;
    LabelFunc info;
    ProgressFunc progress;
    progress.setVisibleFunc = [this] (bool visible) ->void {
        set_progressBarVisible(visible);
    };

    progress.setValueFunc = [this] (int value) ->void {
        set_progressBarValue(value);
    };
    progress.getValueFunc = [this] (void) -> int {
        return progressBarValue();
    };
    progress.getMaximiumFunc = [this] (void) -> int {
        return progressBarMaximium();
    };



    progress.setVisibleFunc(true);
    if (page.setEnabledFunc) {
        page.setEnabledFunc(false);
    }
    if (button.setVisibleFunc) {
        button.setVisibleFunc(false);
    }
    if (info.setVisibleFunc) {
        info.setVisibleFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Please wait, configuring process may take up to 5 minutes"));
    }

    QTimer timer;
    connect(&timer, &QTimer::timeout, [progress](){
        progress.setValueFunc(progress.getValueFunc() + 1);
    });

    progress.setValueFunc(0);
    timer.start(1000);

    ErrorCode e = action();
    qDebug() << "doInstallAction finished with code" << e;

    if (e) {
        if (page.setEnabledFunc) {
            page.setEnabledFunc(true);
        }
        if (button.setVisibleFunc) {
            button.setVisibleFunc(true);
        }
        if (info.setVisibleFunc) {
            info.setVisibleFunc(false);
        }
        QMessageBox::warning(nullptr, APPLICATION_NAME,
                             tr("Error occurred while configuring server.") + "\n" +
                             errorString(e));

        progress.setVisibleFunc(false);
        return e;
    }

    // just ui progressbar tweak
    timer.stop();

    int remaining_val = progress.getMaximiumFunc() - progress.getValueFunc();

    if (remaining_val > 0) {
        QTimer timer1;
        QEventLoop loop1;

        connect(&timer1, &QTimer::timeout, [&](){
            progress.setValueFunc(progress.getValueFunc() + 1);
            if (progress.getValueFunc() >= progress.getMaximiumFunc()) {
                loop1.quit();
            }
        });

        timer1.start(5);
        loop1.exec();
    }


    progress.setVisibleFunc(false);
    if (button.setVisibleFunc) {
        button.setVisibleFunc(true);
    }
    if (page.setEnabledFunc) {
        page.setEnabledFunc(true);
    }
    if (info.setTextFunc) {
        info.setTextFunc(tr("Operation finished"));
    }
    return ErrorCode::NoError;
}
