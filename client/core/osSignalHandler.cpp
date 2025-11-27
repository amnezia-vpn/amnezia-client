#include "osSignalHandler.h"

#include <QCoreApplication>
#include <QSocketNotifier>

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    #include <pthread.h>
    #include <signal.h>
    #include <sys/signalfd.h>
    #include <unistd.h>
#elif defined(Q_OS_MACOS)
    #include <fcntl.h>
    #include <signal.h>
    #include <unistd.h>
#endif

#ifdef Q_OS_WIN
    #include <QMetaObject>
    #include <windows.h>
#endif

namespace
{

    static bool initialized = false;

#ifdef Q_OS_WIN
    static BOOL WINAPI consoleHandler(DWORD signal)
    {
        switch (signal) {
        case CTRL_CLOSE_EVENT:
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            if (QCoreApplication::instance()) {
                QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
            }
            return TRUE;
        default: return FALSE;
        }
    }
#endif

#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
    static int signalFd = -1;
    static QSocketNotifier *socketNotifier = nullptr;

    static void setupUnixSignalHandler()
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGINT);
        sigaddset(&set, SIGTERM);

        pthread_sigmask(SIBLOCK, &set, nullptr);

        signalFd = signalfd(-1, &set, SFD_NONBLOCK | SFD_CLOEXEC);
        if (signalFd < 0)
            return;

        socketNotifier = new QSocketNotifier(signalFd, QSocketNotifier::Read, QCoreApplication::instance());

        QObject::connect(socketNotifier, &QSocketNotifier::activated, QCoreApplication::instance(), [](int) {
            signalfd_siginfo fdsi;
            ::read(signalFd, &fdsi, sizeof(fdsi));

            if (fdsi.ssi_signo == SIGINT || fdsi.ssi_signo == SIGTERM) {
                QCoreApplication::quit();
            }
        });
    }
#elif defined(Q_OS_MACX)
    static int signalPipe[2] = { -1, -1 };
    static QSocketNotifier *socketNotifier = nullptr;

    static void macSignalHandler(int)
    {
        const char ch = 1;
        ::write(signalPipe[1], &ch, sizeof(ch));
    }

    static void setupUnixSignalHandler()
    {
        if (::pipe(signalPipe) != 0)
            return;

        ::fcntl(signalPipe[0], F_SETFL, O_NONBLOCK);
        ::fcntl(signalPipe[1], F_SETFL, O_NONBLOCK);

        struct sigaction sa {};
        sa.sa_handler = macSignalHandler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;

        sigaction(SIGINT, &sa, nullptr);
        sigaction(SIGTERM, &sa, nullptr);

        socketNotifier = new QSocketNotifier(signalPipe[0], QSocketNotifier::Read, QCoreApplication::instance());

        QObject::connect(socketNotifier, &QSocketNotifier::activated, QCoreApplication::instance(), [](int) {
            char buf[16];
            ::read(signalPipe[0], buf, sizeof(buf));
            QCoreApplication::quit();
        });
    }
#endif

    static void cleanupUnixSignalHandler()
    {
#if defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)
        if (socketNotifier) {
            socketNotifier->setEnabled(false);
        }

        if (signalFd >= 0) {
            ::close(signalFd);
            signalFd = -1;
        }

#elif defined(Q_OS_MACOS)
        if (socketNotifier) {
            socketNotifier->setEnabled(false);
        }

        if (signalPipe[0] >= 0) {
            ::close(signalPipe[0]);
            signalPipe[0] = -1;
        }

        if (signalPipe[1] >= 0) {
            ::close(signalPipe[1]);
            signalPipe[1] = -1;
        }
#endif
    }
}

OsSignalHandler::OsSignalHandler(QObject *parent) : QObject(parent)
{
}

void OsSignalHandler::setup()
{
    if (initialized)
        return;

    initialized = true;

#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MACX)
    setupUnixSignalHandler();
#endif

#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(consoleHandler, TRUE);
#endif

    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, [] { cleanupUnixSignalHandler(); });
}
