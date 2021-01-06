/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtservice.h"
#include "qtservice_p.h"
#include <QCoreApplication>
#include <QDateTime>
#include <QFile>
#include <QLibrary>
#include <QMutex>
#include <QSemaphore>
#include <QProcess>
#include <QSettings>
#include <QTextStream>
#include <qt_windows.h>
#include <QWaitCondition>
#include <QAbstractEventDispatcher>
#include <QVector>
#include <QThread>
#if QT_VERSION >= 0x050000
#  include <QAbstractNativeEventFilter>
#endif
#include <stdio.h>
#if defined(QTSERVICE_DEBUG)
#include <QDebug>
#endif

typedef SERVICE_STATUS_HANDLE(WINAPI*PRegisterServiceCtrlHandler)(const wchar_t*,LPHANDLER_FUNCTION);
static PRegisterServiceCtrlHandler pRegisterServiceCtrlHandler = 0;
typedef BOOL(WINAPI*PSetServiceStatus)(SERVICE_STATUS_HANDLE,LPSERVICE_STATUS);
static PSetServiceStatus pSetServiceStatus = 0;
typedef BOOL(WINAPI*PChangeServiceConfig2)(SC_HANDLE,DWORD,LPVOID);
static PChangeServiceConfig2 pChangeServiceConfig2 = 0;
typedef BOOL(WINAPI*PCloseServiceHandle)(SC_HANDLE);
static PCloseServiceHandle pCloseServiceHandle = 0;
typedef SC_HANDLE(WINAPI*PCreateService)(SC_HANDLE,LPCTSTR,LPCTSTR,DWORD,DWORD,DWORD,DWORD,LPCTSTR,LPCTSTR,LPDWORD,LPCTSTR,LPCTSTR,LPCTSTR);
static PCreateService pCreateService = 0;
typedef SC_HANDLE(WINAPI*POpenSCManager)(LPCTSTR,LPCTSTR,DWORD);
static POpenSCManager pOpenSCManager = 0;
typedef BOOL(WINAPI*PDeleteService)(SC_HANDLE);
static PDeleteService pDeleteService = 0;
typedef SC_HANDLE(WINAPI*POpenService)(SC_HANDLE,LPCTSTR,DWORD);
static POpenService pOpenService = 0;
typedef BOOL(WINAPI*PQueryServiceStatus)(SC_HANDLE,LPSERVICE_STATUS);
static PQueryServiceStatus pQueryServiceStatus = 0;
typedef BOOL(WINAPI*PStartServiceCtrlDispatcher)(CONST SERVICE_TABLE_ENTRY*);
static PStartServiceCtrlDispatcher pStartServiceCtrlDispatcher = 0;
typedef BOOL(WINAPI*PStartService)(SC_HANDLE,DWORD,const wchar_t**);
static PStartService pStartService = 0;
typedef BOOL(WINAPI*PControlService)(SC_HANDLE,DWORD,LPSERVICE_STATUS);
static PControlService pControlService = 0;
typedef HANDLE(WINAPI*PDeregisterEventSource)(HANDLE);
static PDeregisterEventSource pDeregisterEventSource = 0;
typedef BOOL(WINAPI*PReportEvent)(HANDLE,WORD,WORD,DWORD,PSID,WORD,DWORD,LPCTSTR*,LPVOID);
static PReportEvent pReportEvent = 0;
typedef HANDLE(WINAPI*PRegisterEventSource)(LPCTSTR,LPCTSTR);
static PRegisterEventSource pRegisterEventSource = 0;
typedef DWORD(WINAPI*PRegisterServiceProcess)(DWORD,DWORD);
static PRegisterServiceProcess pRegisterServiceProcess = 0;
typedef BOOL(WINAPI*PQueryServiceConfig)(SC_HANDLE,LPQUERY_SERVICE_CONFIG,DWORD,LPDWORD);
static PQueryServiceConfig pQueryServiceConfig = 0;
typedef BOOL(WINAPI*PQueryServiceConfig2)(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
static PQueryServiceConfig2 pQueryServiceConfig2 = 0;


#define RESOLVE(name) p##name = (P##name)lib.resolve(#name);
#define RESOLVEA(name) p##name = (P##name)lib.resolve(#name"A");
#define RESOLVEW(name) p##name = (P##name)lib.resolve(#name"W");

static bool winServiceInit()
{
    if (!pOpenSCManager) {
        QLibrary lib("advapi32");

        // only resolve unicode versions
        RESOLVEW(RegisterServiceCtrlHandler);
        RESOLVE(SetServiceStatus);
        RESOLVEW(ChangeServiceConfig2);
        RESOLVE(CloseServiceHandle);
        RESOLVEW(CreateService);
        RESOLVEW(OpenSCManager);
        RESOLVE(DeleteService);
        RESOLVEW(OpenService);
        RESOLVE(QueryServiceStatus);
        RESOLVEW(StartServiceCtrlDispatcher);
        RESOLVEW(StartService); // need only Ansi version
        RESOLVE(ControlService);
        RESOLVE(DeregisterEventSource);
        RESOLVEW(ReportEvent);
        RESOLVEW(RegisterEventSource);
        RESOLVEW(QueryServiceConfig);
        RESOLVEW(QueryServiceConfig2);
    }
    return pOpenSCManager != 0;
}

bool QtServiceController::isInstalled() const
{
    Q_D(const QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, 0);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t*)d->serviceName.utf16(),
                                          SERVICE_QUERY_CONFIG);

        if (hService) {
            result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::isRunning() const
{
    Q_D(const QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, 0);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                                          SERVICE_QUERY_STATUS);
        if (hService) {
            SERVICE_STATUS info;
            int res = pQueryServiceStatus(hService, &info);
            if (res)
                result = info.dwCurrentState != SERVICE_STOPPED;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}


QString QtServiceController::serviceFilePath() const
{
    Q_D(const QtServiceController);
    QString result;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, 0);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                                          SERVICE_QUERY_CONFIG);
        if (hService) {
            DWORD sizeNeeded = 0;
            char data[8 * 1024];
            if (pQueryServiceConfig(hService, (LPQUERY_SERVICE_CONFIG)data, 8 * 1024, &sizeNeeded)) {
                LPQUERY_SERVICE_CONFIG config = (LPQUERY_SERVICE_CONFIG)data;
                result = QString::fromUtf16((const ushort*)config->lpBinaryPathName);
            }
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

QString QtServiceController::serviceDescription() const
{
    Q_D(const QtServiceController);
    QString result;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, 0);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
             SERVICE_QUERY_CONFIG);
        if (hService) {
            DWORD dwBytesNeeded;
            char data[8 * 1024];
            if (pQueryServiceConfig2(
                    hService,
                    SERVICE_CONFIG_DESCRIPTION,
                    (unsigned char *)data,
                    8096,
                    &dwBytesNeeded)) {
                LPSERVICE_DESCRIPTION desc = (LPSERVICE_DESCRIPTION)data;
                if (desc->lpDescription)
                    result = QString::fromUtf16((const ushort*)desc->lpDescription);
            }
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

QtServiceController::StartupType QtServiceController::startupType() const
{
    Q_D(const QtServiceController);
    StartupType result = ManualStartup;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, 0);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                                          SERVICE_QUERY_CONFIG);
        if (hService) {
            DWORD sizeNeeded = 0;
            char data[8 * 1024];
            if (pQueryServiceConfig(hService, (QUERY_SERVICE_CONFIG *)data, 8 * 1024, &sizeNeeded)) {
                QUERY_SERVICE_CONFIG *config = (QUERY_SERVICE_CONFIG *)data;
                result = config->dwStartType == SERVICE_DEMAND_START ? ManualStartup : AutoStartup;
            }
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::uninstall()
{
    Q_D(QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(), DELETE);
        if (hService) {
            if (pDeleteService(hService))
                result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::start(const QStringList &args)
{
    Q_D(QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (hSCM) {
        // Try to open the service
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(), SERVICE_START);
        if (hService) {
            QVector<const wchar_t *> argv(args.size());
            for (int i = 0; i < args.size(); ++i)
                argv[i] = (const wchar_t*)args.at(i).utf16();

            if (pStartService(hService, args.size(), argv.data()))
                result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::stop()
{
    Q_D(QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (hSCM) {
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(), SERVICE_STOP|SERVICE_QUERY_STATUS);
        if (hService) {
            SERVICE_STATUS status;
            if (pControlService(hService, SERVICE_CONTROL_STOP, &status)) {
                bool stopped = status.dwCurrentState == SERVICE_STOPPED;
                int i = 0;
                while(!stopped && i < 10) {
                    Sleep(200);
                    if (!pQueryServiceStatus(hService, &status))
                        break;
                    stopped = status.dwCurrentState == SERVICE_STOPPED;
                    ++i;
                }
                result = stopped;
            } else {
                qErrnoWarning(GetLastError(), "stopping");
            }
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::pause()
{
    Q_D(QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (hSCM) {
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                             SERVICE_PAUSE_CONTINUE);
        if (hService) {
            SERVICE_STATUS status;
            if (pControlService(hService, SERVICE_CONTROL_PAUSE, &status))
                result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::resume()
{
    Q_D(QtServiceController);
    bool result = false;
    if (!winServiceInit())
        return result;

    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (hSCM) {
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                             SERVICE_PAUSE_CONTINUE);
        if (hService) {
            SERVICE_STATUS status;
            if (pControlService(hService, SERVICE_CONTROL_CONTINUE, &status))
                result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

bool QtServiceController::sendCommand(int code)
{
   Q_D(QtServiceController);
   bool result = false;
   if (!winServiceInit())
        return result;

    if (code < 0 || code > 127 || !isRunning())
        return result;

    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_CONNECT);
    if (hSCM) {
        SC_HANDLE hService = pOpenService(hSCM, (wchar_t *)d->serviceName.utf16(),
                                          SERVICE_USER_DEFINED_CONTROL);
        if (hService) {
            SERVICE_STATUS status;
            if (pControlService(hService, 128 + code, &status))
                result = true;
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

#if defined(QTSERVICE_DEBUG)
#  if QT_VERSION >= 0x050000
extern void qtServiceLogDebug(QtMsgType type, const QMessageLogContext &context, const QString &msg);
#  else
extern void qtServiceLogDebug(QtMsgType type, const char* msg);
#  endif
#endif

void QtServiceBase::logMessage(const QString &message, MessageType type,
                           int id, uint category, const QByteArray &data)
{
#if defined(QTSERVICE_DEBUG)
    QByteArray dbgMsg("[LOGGED ");
    switch (type) {
    case Error: dbgMsg += "Error] " ; break;
    case Warning: dbgMsg += "Warning] "; break;
    case Success: dbgMsg += "Success] "; break;
    case Information: //fall through
    default: dbgMsg += "Information] "; break;
    }
#  if QT_VERSION >= 0x050000
    qtServiceLogDebug((QtMsgType)-1, QMessageLogContext(), QLatin1String(dbgMsg) + message);
#  else
    qtServiceLogDebug((QtMsgType)-1, (dbgMsg + message.toAscii()).constData());
#  endif
#endif

    Q_D(QtServiceBase);
    if (!winServiceInit())
        return;
    WORD wType;
    switch (type) {
    case Error: wType = EVENTLOG_ERROR_TYPE; break;
    case Warning: wType = EVENTLOG_WARNING_TYPE; break;
    case Information: wType = EVENTLOG_INFORMATION_TYPE; break;
    default: wType = EVENTLOG_SUCCESS; break;
    }
    HANDLE h = pRegisterEventSource(0, (wchar_t *)d->controller.serviceName().utf16());
    if (h) {
        const wchar_t *msg = (wchar_t*)message.utf16();
        const char *bindata = data.size() ? data.constData() : 0;
        pReportEvent(h, wType, category, id, 0, 1, data.size(),(const wchar_t **)&msg,
                     const_cast<char *>(bindata));
        pDeregisterEventSource(h);
    }
}

class QtServiceControllerHandler : public QObject
{
    Q_OBJECT
public:
    QtServiceControllerHandler(QtServiceSysPrivate *sys);

protected:
    void customEvent(QEvent *e);

private:
    QtServiceSysPrivate *d_sys;
};

class QtServiceSysPrivate
{
public:
    enum {
        QTSERVICE_STARTUP = 256
    };
    QtServiceSysPrivate();

    void setStatus( DWORD dwState );
    void setServiceFlags(QtServiceBase::ServiceFlags flags);
    DWORD serviceFlags(QtServiceBase::ServiceFlags flags) const;
    inline bool available() const;
    static void WINAPI serviceMain( DWORD dwArgc, wchar_t** lpszArgv );
    static void WINAPI handler( DWORD dwOpcode );

    SERVICE_STATUS status;
    SERVICE_STATUS_HANDLE serviceStatus;
    QStringList serviceArgs;

    static QtServiceSysPrivate *instance;
#if QT_VERSION < 0x050000
    static QCoreApplication::EventFilter nextFilter;
#endif

    QWaitCondition condition;
    QMutex mutex;
    QSemaphore startSemaphore;
    QSemaphore startSemaphore2;

    QtServiceControllerHandler *controllerHandler;

    void handleCustomEvent(QEvent *e);
};

QtServiceControllerHandler::QtServiceControllerHandler(QtServiceSysPrivate *sys)
    : QObject(), d_sys(sys)
{

}

void QtServiceControllerHandler::customEvent(QEvent *e)
{
    d_sys->handleCustomEvent(e);
}


QtServiceSysPrivate *QtServiceSysPrivate::instance = 0;
#if QT_VERSION < 0x050000
QCoreApplication::EventFilter QtServiceSysPrivate::nextFilter = 0;
#endif

QtServiceSysPrivate::QtServiceSysPrivate()
{
    instance = this;
}

inline bool QtServiceSysPrivate::available() const
{
    return 0 != pOpenSCManager;
}

void WINAPI QtServiceSysPrivate::serviceMain(DWORD dwArgc, wchar_t** lpszArgv)
{
    if (!instance || !QtServiceBase::instance())
        return;

    // Windows spins off a random thread to call this function on
    // startup, so here we just signal to the QApplication event loop
    // in the main thread to go ahead with start()'ing the service.

    for (DWORD i = 0; i < dwArgc; i++)
        instance->serviceArgs.append(QString::fromUtf16((unsigned short*)lpszArgv[i]));

    instance->startSemaphore.release(); // let the qapp creation start
    instance->startSemaphore2.acquire(); // wait until its done
    // Register the control request handler
    instance->serviceStatus = pRegisterServiceCtrlHandler((TCHAR*)QtServiceBase::instance()->serviceName().utf16(), handler);

    if (!instance->serviceStatus) // cannot happen - something is utterly wrong
        return;

    handler(QTSERVICE_STARTUP); // Signal startup to the application -
                                // causes QtServiceBase::start() to be called in the main thread

    // The MSDN doc says that this thread should just exit - the service is
    // running in the main thread (here, via callbacks in the handler thread).
}


// The handler() is called from the thread that called
// StartServiceCtrlDispatcher, i.e. our HandlerThread, and
// not from the main thread that runs the event loop, so we
// have to post an event to ourselves, and use a QWaitCondition
// and a QMutex to synchronize.
void QtServiceSysPrivate::handleCustomEvent(QEvent *e)
{
    int code = e->type() - QEvent::User;

    switch(code) {
    case QTSERVICE_STARTUP: // Startup
        QtServiceBase::instance()->start();
        break;
    case SERVICE_CONTROL_STOP:
        QtServiceBase::instance()->stop();
        QCoreApplication::instance()->quit();
        break;
    case SERVICE_CONTROL_PAUSE:
        QtServiceBase::instance()->pause();
        break;
    case SERVICE_CONTROL_CONTINUE:
        QtServiceBase::instance()->resume();
        break;
    default:
	if (code >= 128 && code <= 255)
	    QtServiceBase::instance()->processCommand(code - 128);
        break;
    }

    mutex.lock();
    condition.wakeAll();
    mutex.unlock();
}

void WINAPI QtServiceSysPrivate::handler( DWORD code )
{
    if (!instance)
        return;

    instance->mutex.lock();
    switch (code) {
    case QTSERVICE_STARTUP: // QtService startup (called from WinMain when started)
        instance->setStatus(SERVICE_START_PENDING);
        QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + code)));
        instance->condition.wait(&instance->mutex);
        instance->setStatus(SERVICE_RUNNING);
        break;
    case SERVICE_CONTROL_STOP: // 1
        instance->setStatus(SERVICE_STOP_PENDING);
        QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + code)));
        instance->condition.wait(&instance->mutex);
        // status will be reported as stopped by start() when qapp::exec returns
        break;

    case SERVICE_CONTROL_PAUSE: // 2
        instance->setStatus(SERVICE_PAUSE_PENDING);
        QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + code)));
        instance->condition.wait(&instance->mutex);
        instance->setStatus(SERVICE_PAUSED);
        break;

    case SERVICE_CONTROL_CONTINUE: // 3
        instance->setStatus(SERVICE_CONTINUE_PENDING);
        QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + code)));
        instance->condition.wait(&instance->mutex);
        instance->setStatus(SERVICE_RUNNING);
        break;

    case SERVICE_CONTROL_INTERROGATE: // 4
        break;

    case SERVICE_CONTROL_SHUTDOWN: // 5
        // Don't waste time with reporting stop pending, just do it
        QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + SERVICE_CONTROL_STOP)));
        instance->condition.wait(&instance->mutex);
        // status will be reported as stopped by start() when qapp::exec returns
        break;

    default:
        if ( code >= 128 && code <= 255 ) {
            QCoreApplication::postEvent(instance->controllerHandler, new QEvent(QEvent::Type(QEvent::User + code)));
            instance->condition.wait(&instance->mutex);
        }
        break;
    }

    instance->mutex.unlock();

    // Report current status
    if (instance->available() && instance->status.dwCurrentState != SERVICE_STOPPED)
        pSetServiceStatus(instance->serviceStatus, &instance->status);
}

void QtServiceSysPrivate::setStatus(DWORD state)
{
    if (!available())
	return;
    status.dwCurrentState = state;
    pSetServiceStatus(serviceStatus, &status);
}

void QtServiceSysPrivate::setServiceFlags(QtServiceBase::ServiceFlags flags)
{
    if (!available())
        return;
    status.dwControlsAccepted = serviceFlags(flags);
    pSetServiceStatus(serviceStatus, &status);
}

DWORD QtServiceSysPrivate::serviceFlags(QtServiceBase::ServiceFlags flags) const
{
    DWORD control = 0;
    if (flags & QtServiceBase::CanBeSuspended)
        control |= SERVICE_ACCEPT_PAUSE_CONTINUE;
    if (!(flags & QtServiceBase::CannotBeStopped))
        control |= SERVICE_ACCEPT_STOP;
    if (flags & QtServiceBase::NeedsStopOnShutdown)
        control |= SERVICE_ACCEPT_SHUTDOWN;

    return control;
}

#include "qtservice_win.moc"


class HandlerThread : public QThread
{
public:
    HandlerThread()
        : success(true), console(false), QThread()
        {}

    bool calledOk() { return success; }
    bool runningAsConsole() { return console; }

protected:
    bool success, console;
    void run()
        {
            SERVICE_TABLE_ENTRYW st [2];
            st[0].lpServiceName = (wchar_t*)QtServiceBase::instance()->serviceName().utf16();
            st[0].lpServiceProc = QtServiceSysPrivate::serviceMain;
            st[1].lpServiceName = 0;
            st[1].lpServiceProc = 0;

            success = (pStartServiceCtrlDispatcher(st) != 0); // should block

            if (!success) {
                if (GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
                    // Means we're started from console, not from service mgr
                    // start() will ask the mgr to start another instance of us as a service instead
                    console = true;
                }
                else {
                    QtServiceBase::instance()->logMessage(QString("The Service failed to start [%1]").arg(qt_error_string(GetLastError())), QtServiceBase::Error);
                }
                QtServiceSysPrivate::instance->startSemaphore.release();  // let start() continue, since serviceMain won't be doing it
            }
        }
};

/*
  Ignore WM_ENDSESSION system events, since they make the Qt kernel quit
*/

#if QT_VERSION >= 0x050000

class QtServiceAppEventFilter : public QAbstractNativeEventFilter
{
public:
    QtServiceAppEventFilter() {}
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result);
};

bool QtServiceAppEventFilter::nativeEventFilter(const QByteArray &, void *message, long *result)
{
    MSG *winMessage = (MSG*)message;
    if (winMessage->message == WM_ENDSESSION && (winMessage->lParam & ENDSESSION_LOGOFF)) {
        *result = TRUE;
        return true;
    }
    return false;
}

Q_GLOBAL_STATIC(QtServiceAppEventFilter, qtServiceAppEventFilter)

#else

bool myEventFilter(void* message, long* result)
{
    MSG* msg = reinterpret_cast<MSG*>(message);
    if (!msg || (msg->message != WM_ENDSESSION) || !(msg->lParam & ENDSESSION_LOGOFF))
        return QtServiceSysPrivate::nextFilter ? QtServiceSysPrivate::nextFilter(message, result) : false;

    if (QtServiceSysPrivate::nextFilter)
        QtServiceSysPrivate::nextFilter(message, result);
    if (result)
        *result = TRUE;
    return true;
}

#endif

/* There are three ways we can be started:

   - By a service controller (e.g. the Services control panel), with
   no (service-specific) arguments. ServiceBase::exec() will then call
   start() below, and the service will start.

   - From the console, but with no (service-specific) arguments. This
   means we should ask a controller to start the service (i.e. another
   instance of this executable), and then just terminate. We discover
   this case (as different from the above) by the fact that
   StartServiceCtrlDispatcher will return an error, instead of blocking.

   - From the console, with -e(xec) argument. ServiceBase::exec() will
   then call ServiceBasePrivate::exec(), which calls
   ServiceBasePrivate::run(), which runs the application as a normal
   program.
*/

bool QtServiceBasePrivate::start()
{
    sysInit();
    if (!winServiceInit())
        return false;

    // Since StartServiceCtrlDispatcher() blocks waiting for service
    // control events, we need to call it in another thread, so that
    // the main thread can run the QApplication event loop.
    HandlerThread* ht = new HandlerThread();
    ht->start();

    QtServiceSysPrivate* sys = QtServiceSysPrivate::instance;

    // Wait until service args have been received by serviceMain.
    // If Windows doesn't call serviceMain (or
    // StartServiceControlDispatcher doesn't return an error) within
    // a timeout of 20 secs, something is very wrong; give up
    if (!sys->startSemaphore.tryAcquire(1, 20000))
        return false;

    if (!ht->calledOk()) {
        if (ht->runningAsConsole())
            return controller.start(args.mid(1));
        else
            return false;
    }

    int argc = sys->serviceArgs.size();
    QVector<char *> argv(argc);
    QList<QByteArray> argvData;
    for (int i = 0; i < argc; ++i)
        argvData.append(sys->serviceArgs.at(i).toLocal8Bit());
    for (int i = 0; i < argc; ++i)
        argv[i] = argvData[i].data();

    q_ptr->createApplication(argc, argv.data());
    QCoreApplication *app = QCoreApplication::instance();
    if (!app)
        return false;

#if QT_VERSION >= 0x050000
    QAbstractEventDispatcher::instance()->installNativeEventFilter(qtServiceAppEventFilter());
#else
    QtServiceSysPrivate::nextFilter = app->setEventFilter(myEventFilter);
#endif

    sys->controllerHandler = new QtServiceControllerHandler(sys);

    sys->startSemaphore2.release(); // let serviceMain continue (and end)

    sys->status.dwWin32ExitCode = q_ptr->executeApplication();
    sys->setStatus(SERVICE_STOPPED);

    if (ht->isRunning())
        ht->wait(1000);         // let the handler thread finish
    delete sys->controllerHandler;
    sys->controllerHandler = 0;
    if (ht->isFinished())
        delete ht;
    delete app;
    sysCleanup();
    return true;
}

bool QtServiceBasePrivate::install(const QString &account, const QString &password)
{
    bool result = false;
    if (!winServiceInit())
        return result;

    // Open the Service Control Manager
    SC_HANDLE hSCM = pOpenSCManager(0, 0, SC_MANAGER_ALL_ACCESS);
    if (hSCM) {
        QString acc = account;
        DWORD dwStartType = startupType == QtServiceController::AutoStartup ? SERVICE_AUTO_START : SERVICE_DEMAND_START;
        DWORD dwServiceType = SERVICE_WIN32_OWN_PROCESS;
        wchar_t *act = 0;
        wchar_t *pwd = 0;
        if (!acc.isEmpty()) {
            // The act string must contain a string of the format "Domain\UserName",
            // so if only a username was specified without a domain, default to the local machine domain.
            if (!acc.contains(QChar('\\'))) {
                acc.prepend(QLatin1String(".\\"));
            }
            if (!acc.endsWith(QLatin1String("\\LocalSystem")))
                act = (wchar_t*)acc.utf16();
        }
        if (!password.isEmpty() && act) {
            pwd = (wchar_t*)password.utf16();
        }

        // Only set INTERACTIVE if act is LocalSystem. (and act should be 0 if it is LocalSystem).
        if (!act) dwServiceType |= SERVICE_INTERACTIVE_PROCESS;

        // Create the service
        SC_HANDLE hService = pCreateService(hSCM, (wchar_t *)controller.serviceName().utf16(),
                                            (wchar_t *)controller.serviceName().utf16(),
                                            SERVICE_ALL_ACCESS,
                                            dwServiceType, // QObject::inherits ( const char * className ) for no inter active ????
                                            dwStartType, SERVICE_ERROR_NORMAL, (wchar_t *)filePath().utf16(),
                                            0, 0, 0,
                                            act, pwd);
        if (hService) {
            result = true;
            if (!serviceDescription.isEmpty()) {
                SERVICE_DESCRIPTION sdesc;
                sdesc.lpDescription = (wchar_t *)serviceDescription.utf16();
                pChangeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &sdesc);
            }
            pCloseServiceHandle(hService);
        }
        pCloseServiceHandle(hSCM);
    }
    return result;
}

QString QtServiceBasePrivate::filePath() const
{
    wchar_t path[_MAX_PATH];
    ::GetModuleFileNameW( 0, path, sizeof(path) );
    return QString::fromUtf16((unsigned short*)path);
}

bool QtServiceBasePrivate::sysInit()
{
    sysd = new QtServiceSysPrivate();

    sysd->serviceStatus			    = 0;
    sysd->status.dwServiceType		    = SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS;
    sysd->status.dwCurrentState		    = SERVICE_STOPPED;
    sysd->status.dwControlsAccepted         = sysd->serviceFlags(serviceFlags);
    sysd->status.dwWin32ExitCode	    = NO_ERROR;
    sysd->status.dwServiceSpecificExitCode  = 0;
    sysd->status.dwCheckPoint		    = 0;
    sysd->status.dwWaitHint		    = 0;

    return true;
}

void QtServiceBasePrivate::sysSetPath()
{

}

void QtServiceBasePrivate::sysCleanup()
{
    if (sysd) {
        delete sysd;
        sysd = 0;
    }
}

void QtServiceBase::setServiceFlags(QtServiceBase::ServiceFlags flags)
{
    if (d_ptr->serviceFlags == flags)
        return;
    d_ptr->serviceFlags = flags;
    if (d_ptr->sysd)
        d_ptr->sysd->setServiceFlags(flags);
}


