#include "windowsUtil.h"

#include <QGuiApplication>
#include <QWindow>

#include <windows.h>

void activateApplication()
{
    QWindow *window = nullptr;
    for (QWindow *candidate : QGuiApplication::topLevelWindows()) {
        if (candidate->objectName() == QLatin1String("mainWindow")) {
            window = candidate;
            break;
        }
    }
    if (!window) {
        return;
    }

    if (window->visibility() == QWindow::Hidden) {
        window->show();
    }

    const HWND hwnd = reinterpret_cast<HWND>(window->winId());
    if (!hwnd) {
        return;
    }
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }

    const HWND foreground = GetForegroundWindow();
    if (foreground == hwnd) {
        return;
    }

    const DWORD foregroundThread = foreground ? GetWindowThreadProcessId(foreground, nullptr) : 0;
    const DWORD currentThread = GetCurrentThreadId();
    const bool attached = foregroundThread && foregroundThread != currentThread
            && AttachThreadInput(currentThread, foregroundThread, TRUE);
    SetForegroundWindow(hwnd);
    if (attached) {
        AttachThreadInput(currentThread, foregroundThread, FALSE);
    }
}
