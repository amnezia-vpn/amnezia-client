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
#include <stdio.h>
#include <QTimer>
#include <QVector>
#include <QProcess>

#if defined(QTSERVICE_DEBUG)
#include <QDebug>
#include <QString>
#include <QFile>
#include <QTime>
#include <QMutex>
#if defined(Q_OS_WIN32)
#include <qt_windows.h>
#else
#include <unistd.h>
#include <stdlib.h>
#endif

static QFile* f = 0;

static void qtServiceCloseDebugLog()
{
    if (!f)
        return;
    f->write(QTime::currentTime().toString("HH:mm:ss.zzz").toLatin1());
    f->write(" --- DEBUG LOG CLOSED ---\n\n");
    f->flush();
    f->close();
    delete f;
    f = 0;
}

#if QT_VERSION >= 0x050000
void qtServiceLogDebug(QtMsgType type, const QMessageLogContext &context, const QString &msg)
#else
void qtServiceLogDebug(QtMsgType type, const char* msg)
#endif
{
    static QMutex mutex;
    QMutexLocker locker(&mutex);
#if defined(Q_OS_WIN32)
    const qulonglong processId = GetCurrentProcessId();
#else
    const qulonglong processId = getpid();
#endif
    QByteArray s(QTime::currentTime().toString("HH:mm:ss.zzz").toLatin1());
    s += " [";
    s += QByteArray::number(processId);
    s += "] ";

    if (!f) {
#if defined(Q_OS_WIN32)
        f = new QFile("c:/service-debuglog.txt");
#else
        f = new QFile("/tmp/service-debuglog.txt");
#endif
        if (!f->open(QIODevice::WriteOnly | QIODevice::Append)) {
            delete f;
            f = 0;
            return;
        }
        QByteArray ps('\n' + s + "--- DEBUG LOG OPENED ---\n");
        f->write(ps);
    }

    switch (type) {
    case QtWarningMsg:
        s += "WARNING: ";
        break;
    case QtCriticalMsg:
        s += "CRITICAL: ";
        break;
    case QtFatalMsg:
        s+= "FATAL: ";
        break;
    case QtDebugMsg:
        s += "DEBUG: ";
        break;
    default:
        // Nothing
        break;
    }

#if QT_VERSION >= 0x050400
    s += qFormatLogMessage(type, context, msg).toLocal8Bit();
#elif QT_VERSION >= 0x050000
    s += msg.toLocal8Bit();
    Q_UNUSED(context)
#else
    s += msg;
#endif
    s += '\n';

    f->write(s);
    f->flush();

    if (type == QtFatalMsg) {
        qtServiceCloseDebugLog();
        exit(1);
    }
}

#endif

/*!
    \class QtServiceController

    \brief The QtServiceController class allows you to control
    services from separate applications.

    QtServiceController provides a collection of functions that lets
    you install and run a service controlling its execution, as well
    as query its status.

    In order to run a service, the service must be installed in the
    system's service database using the install() function. The system
    will start the service depending on the specified StartupType; it
    can either be started during system startup, or when a process
    starts it manually.

    Once a service is installed, the service can be run and controlled
    manually using the start(), stop(), pause(), resume() or
    sendCommand() functions.  You can at any time query for the
    service's status using the isInstalled() and isRunning()
    functions, or you can query its properties using the
    serviceDescription(), serviceFilePath(), serviceName() and
    startupType() functions. For example:

    \code
    MyService service;       \\ which inherits QtService
    QString serviceFilePath;

    QtServiceController controller(service.serviceName());

    if (controller.install(serviceFilePath))
        controller.start()

    if (controller.isRunning())
        QMessageBox::information(this, tr("Service Status"),
                                 tr("The %1 service is started").arg(controller.serviceName()));

    ...

    controller.stop();
    controller.uninstall();
    }
    \endcode

    An instance of the service controller can only control one single
    service. To control several services within one application, you
    must create en equal number of service controllers.

    The QtServiceController destructor neither stops nor uninstalls
    the associated service. To stop a service the stop() function must
    be called explicitly. To uninstall a service, you can use the
    uninstall() function.

    \sa QtServiceBase, QtService
*/

/*!
    \enum QtServiceController::StartupType
    This enum describes when a service should be started.

    \value AutoStartup The service is started during system startup.
    \value ManualStartup The service must be started manually by a process.

    \warning The \a StartupType enum is ignored under UNIX-like
    systems. A service, or daemon, can only be started manually on such
    systems with current implementation.

    \sa startupType()
*/


/*!
    Creates a controller object for the service with the given
    \a name.
*/
QtServiceController::QtServiceController(const QString &name)
 : d_ptr(new QtServiceControllerPrivate())
{
    Q_D(QtServiceController);
    d->q_ptr = this;
    d->serviceName = name;
}
/*!
    Destroys the service controller. This neither stops nor uninstalls
    the controlled service.

    To stop a service the stop() function must be called
    explicitly. To uninstall a service, you can use the uninstall()
    function.

    \sa stop(), QtServiceController::uninstall()
*/
QtServiceController::~QtServiceController()
{
    delete d_ptr;
}
/*!
    \fn bool QtServiceController::isInstalled() const

    Returns true if the service is installed; otherwise returns false.

    On Windows it uses the system's service control manager.

    On Unix it checks configuration written to QSettings::SystemScope
    using "QtSoftware" as organization name.

    \sa install()
*/

/*!
    \fn bool QtServiceController::isRunning() const

    Returns true if the service is running; otherwise returns false. A
    service must be installed before it can be run using a controller.

    \sa start(), isInstalled()
*/

/*!
    Returns the name of the controlled service.

    \sa QtServiceController(), serviceDescription()
*/
QString QtServiceController::serviceName() const
{
    Q_D(const QtServiceController);
    return d->serviceName;
}
/*!
    \fn QString QtServiceController::serviceDescription() const

    Returns the description of the controlled service.

    \sa install(), serviceName()
*/

/*!
    \fn QtServiceController::StartupType QtServiceController::startupType() const

    Returns the startup type of the controlled service.

    \sa install(), serviceName()
*/

/*!
    \fn QString QtServiceController::serviceFilePath() const

    Returns the file path to the controlled service.

    \sa install(), serviceName()
*/

/*!
    Installs the service with the given \a serviceFilePath
    and returns true if the service is installed
    successfully; otherwise returns false.

    On Windows service is installed in the system's service control manager with the given
    \a account and \a password.

    On Unix service configuration is written to QSettings::SystemScope
    using "QtSoftware" as organization name. \a account and \a password
    arguments are ignored.

    \warning Due to the different implementations of how services (daemons)
    are installed on various UNIX-like systems, this method doesn't
    integrate the service into the system's startup scripts.

    \sa uninstall(), start()
*/
bool QtServiceController::install(const QString &serviceFilePath, const QString &account,
                const QString &password)
{
    QStringList arguments;
    arguments << QLatin1String("-i");
    arguments << account;
    arguments << password;
    return (QProcess::execute(serviceFilePath, arguments) == 0);
}


/*!
    \fn bool QtServiceController::uninstall()

    Uninstalls the service and returns true if successful; otherwise returns false.

    On Windows service is uninstalled using the system's service control manager.

    On Unix service configuration is cleared using QSettings::SystemScope
    with "QtSoftware" as organization name.


    \sa install()
*/

/*!
    \fn bool QtServiceController::start(const QStringList &arguments)

    Starts the installed service passing the given \a arguments to the
    service. A service must be installed before a controller can run it.

    Returns true if the service could be started; otherwise returns
    false.

    \sa install(), stop()
*/

/*!
    \overload

    Starts the installed service without passing any arguments to the service.
*/
bool QtServiceController::start()
{
    return start(QStringList());
}

/*!
    \fn bool QtServiceController::stop()

    Requests the running service to stop. The service will call the
    QtServiceBase::stop() implementation unless the service's state
    is QtServiceBase::CannotBeStopped.  This function does nothing if
    the service is not running.

    Returns true if a running service was successfully stopped;
    otherwise false.

    \sa start(), QtServiceBase::stop(), QtServiceBase::ServiceFlags
*/

/*!
    \fn bool QtServiceController::pause()

    Requests the running service to pause. If the service's state is
    QtServiceBase::CanBeSuspended, the service will call the
    QtServiceBase::pause() implementation. The function does nothing
    if the service is not running.

    Returns true if a running service was successfully paused;
    otherwise returns false.

    \sa resume(), QtServiceBase::pause(), QtServiceBase::ServiceFlags
*/

/*!
    \fn bool QtServiceController::resume()

    Requests the running service to continue. If the service's state
    is QtServiceBase::CanBeSuspended, the service will call the
    QtServiceBase::resume() implementation. This function does nothing
    if the service is not running.

    Returns true if a running service was successfully resumed;
    otherwise returns false.

    \sa pause(), QtServiceBase::resume(), QtServiceBase::ServiceFlags
*/

/*!
    \fn bool QtServiceController::sendCommand(int code)

    Sends the user command \a code to the service. The service will
    call the QtServiceBase::processCommand() implementation.  This
    function does nothing if the service is not running.

    Returns true if the request was sent to a running service;
    otherwise returns false.

    \sa QtServiceBase::processCommand()
*/

class QtServiceStarter : public QObject
{
    Q_OBJECT
public:
    QtServiceStarter(QtServiceBasePrivate *service)
        : QObject(), d_ptr(service) {}
public slots:
    void slotStart()
    {
        d_ptr->startService();
    }
private:
    QtServiceBasePrivate *d_ptr;
};
#include "qtservice.moc"

QtServiceBase *QtServiceBasePrivate::instance = 0;

QtServiceBasePrivate::QtServiceBasePrivate(const QString &name)
    : startupType(QtServiceController::ManualStartup), serviceFlags(0), controller(name)
{

}

QtServiceBasePrivate::~QtServiceBasePrivate()
{

}

void QtServiceBasePrivate::startService()
{
    q_ptr->start();
}

int QtServiceBasePrivate::run(bool asService, const QStringList &argList)
{
    int argc = argList.size();
    QVector<char *> argv(argc);
    QList<QByteArray> argvData;
    for (int i = 0; i < argc; ++i)
        argvData.append(argList.at(i).toLocal8Bit());
    for (int i = 0; i < argc; ++i)
        argv[i] = argvData[i].data();

    if (asService && !sysInit())
        return -1;

    q_ptr->createApplication(argc, argv.data());
    QCoreApplication *app = QCoreApplication::instance();
    if (!app)
        return -1;

    if (asService)
        sysSetPath();

    QtServiceStarter starter(this);
    QTimer::singleShot(0, &starter, SLOT(slotStart()));
    int res = q_ptr->executeApplication();
    delete app;

    if (asService)
        sysCleanup();
    return res;
}


/*!
    \class QtServiceBase

    \brief The QtServiceBase class provides an API for implementing
    Windows services and Unix daemons.

    A Windows service or Unix daemon (a "service"), is a program that
    runs "in the background" independently of whether a user is logged
    in or not. A service is often set up to start when the machine
    boots up, and will typically run continuously as long as the
    machine is on.

    Services are usually non-interactive console applications. User
    interaction, if required, is usually implemented in a separate,
    normal GUI application that communicates with the service through
    an IPC channel. For simple communication,
    QtServiceController::sendCommand() and QtService::processCommand()
    may be used, possibly in combination with a shared settings
    file. For more complex, interactive communication, a custom IPC
    channel should be used, e.g. based on Qt's networking classes. (In
    certain circumstances, a service may provide a GUI itself,
    ref. the "interactive" example documentation).

    Typically, you will create a service by subclassing the QtService
    template class which inherits QtServiceBase and allows you to
    create a service for a particular application type.

    The Windows implementation uses the NT Service Control Manager,
    and the application can be controlled through the system
    administration tools. Services are usually launched using the
    system account, which requires that all DLLs that the service
    executable depends on (i.e. Qt), are located in the same directory
    as the service, or in a system path.

    On Unix a service is implemented as a daemon.

    You can retrieve the service's description, state, and startup
    type using the serviceDescription(), serviceFlags() and
    startupType() functions respectively. The service's state is
    decribed by the ServiceFlag enum. The mentioned properites can
    also be set using the corresponding set functions. In addition you
    can retrieve the service's name using the serviceName() function.

    Several of QtServiceBase's protected functions are called on
    requests from the QtServiceController class:

    \list
        \o start()
        \o pause()
        \o processCommand()
        \o resume()
        \o stop()
    \endlist

    You can control any given service using an instance of the
    QtServiceController class which also allows you to control
    services from separate applications. The mentioned functions are
    all virtual and won't do anything unless they are
    reimplemented. You can reimplement these functions to pause and
    resume the service's execution, as well as process user commands
    and perform additional clean-ups before shutting down.

    QtServiceBase also provides the static instance() function which
    returns a pointer to an application's QtServiceBase instance. In
    addition, a service can report events to the system's event log
    using the logMessage() function. The MessageType enum describes
    the different types of messages a service reports.

    The implementation of a service application's main function
    typically creates an service object derived by subclassing the
    QtService template class. Then the main function will call this
    service's exec() function, and return the result of that call. For
    example:

    \code
        int main(int argc, char **argv)
        {
            MyService service(argc, argv);
            return service.exec();
        }
    \endcode

    When the exec() function is called, it will parse the service
    specific arguments passed in \c argv, perform the required
    actions, and return.

    \target serviceSpecificArguments

    The following arguments are recognized as service specific:

    \table
    \header \i Short \i Long \i Explanation
    \row \i -i \i -install \i Install the service.
    \row \i -u \i -uninstall \i Uninstall the service.
    \row \i -e \i -exec
         \i Execute the service as a standalone application (useful for debug purposes).
            This is a blocking call, the service will be executed like a normal application.
            In this mode you will not be able to communicate with the service from the contoller.
    \row \i -t \i -terminate \i Stop the service.
    \row \i -p \i -pause \i Pause the service.
    \row \i -r \i -resume \i Resume a paused service.
    \row \i -c \e{cmd} \i -command \e{cmd}
	 \i Send the user defined command code \e{cmd} to the service application.
    \row \i -v \i -version \i Display version and status information.
    \endtable

    If \e none of the arguments is recognized as service specific,
    exec() will first call the createApplication() function, then
    executeApplication() and finally the start() function. In the end,
    exec() returns while the service continues in its own process
    waiting for commands from the service controller.

    \sa QtService, QtServiceController
*/

/*!
    \enum QtServiceBase::MessageType

    This enum describes the different types of messages a service
    reports to the system log.

    \value Success An operation has succeeded, e.g. the service
           is started.
    \value Error An operation failed, e.g. the service failed to start.
    \value Warning An operation caused a warning that might require user
           interaction.
    \value Information Any type of usually non-critical information.
*/

/*!
    \enum QtServiceBase::ServiceFlag

    This enum describes the different capabilities of a service.

    \value Default The service can be stopped, but not suspended.
    \value CanBeSuspended The service can be suspended.
    \value CannotBeStopped The service cannot be stopped.
    \value NeedsStopOnShutdown (Windows only) The service will be stopped before the system shuts down. Note that Microsoft recommends this only for services that must absolutely clean up during shutdown, because there is a limited time available for shutdown of services.
*/

/*!
    Creates a service instance called \a name. The \a argc and \a argv
    parameters are parsed after the exec() function has been
    called. Then they are passed to the application's constructor.
    The application type is determined by the QtService subclass.

    The service is neither installed nor started. The name must not
    contain any backslashes or be longer than 255 characters. In
    addition, the name must be unique in the system's service
    database.

    \sa exec(), start(), QtServiceController::install()
*/
QtServiceBase::QtServiceBase(int argc, char **argv, const QString &name)
{
#if defined(QTSERVICE_DEBUG)
#  if QT_VERSION >= 0x050000
    qInstallMessageHandler(qtServiceLogDebug);
#  else
    qInstallMsgHandler(qtServiceLogDebug);
#  endif
    qAddPostRoutine(qtServiceCloseDebugLog);
#endif

    Q_ASSERT(!QtServiceBasePrivate::instance);
    QtServiceBasePrivate::instance = this;

    QString nm(name);
    if (nm.length() > 255) {
	qWarning("QtService: 'name' is longer than 255 characters.");
	nm.truncate(255);
    }
    if (nm.contains('\\')) {
	qWarning("QtService: 'name' contains backslashes '\\'.");
	nm.replace((QChar)'\\', (QChar)'\0');
    }

    d_ptr = new QtServiceBasePrivate(nm);
    d_ptr->q_ptr = this;

    d_ptr->serviceFlags = 0;
    d_ptr->sysd = 0;
    for (int i = 0; i < argc; ++i)
        d_ptr->args.append(QString::fromLocal8Bit(argv[i]));
}

/*!
    Destroys the service object. This neither stops nor uninstalls the
    service.

    To stop a service the stop() function must be called
    explicitly. To uninstall a service, you can use the
    QtServiceController::uninstall() function.

    \sa stop(), QtServiceController::uninstall()
*/
QtServiceBase::~QtServiceBase()
{
    delete d_ptr;
    QtServiceBasePrivate::instance = 0;
}

/*!
    Returns the name of the service.

    \sa QtServiceBase(), serviceDescription()
*/
QString QtServiceBase::serviceName() const
{
    return d_ptr->controller.serviceName();
}

/*!
    Returns the description of the service.

    \sa setServiceDescription(), serviceName()
*/
QString QtServiceBase::serviceDescription() const
{
    return d_ptr->serviceDescription;
}

/*!
    Sets the description of the service to the given \a description.

    \sa serviceDescription()
*/
void QtServiceBase::setServiceDescription(const QString &description)
{
    d_ptr->serviceDescription = description;
}

/*!
    Returns the service's startup type.

    \sa QtServiceController::StartupType, setStartupType()
*/
QtServiceController::StartupType QtServiceBase::startupType() const
{
    return d_ptr->startupType;
}

/*!
    Sets the service's startup type to the given \a type.

    \sa QtServiceController::StartupType, startupType()
*/
void QtServiceBase::setStartupType(QtServiceController::StartupType type)
{
    d_ptr->startupType = type;
}

/*!
    Returns the service's state which is decribed using the
    ServiceFlag enum.

    \sa ServiceFlags, setServiceFlags()
*/
QtServiceBase::ServiceFlags QtServiceBase::serviceFlags() const
{
    return d_ptr->serviceFlags;
}

/*!
    \fn void QtServiceBase::setServiceFlags(ServiceFlags flags)

    Sets the service's state to the state described by the given \a
    flags.

    \sa ServiceFlags, serviceFlags()
*/

/*!
    Executes the service.

    When the exec() function is called, it will parse the \l
    {serviceSpecificArguments} {service specific arguments} passed in
    \c argv, perform the required actions, and exit.

    If none of the arguments is recognized as service specific, exec()
    will first call the createApplication() function, then executeApplication() and
    finally the start() function. In the end, exec()
    returns while the service continues in its own process waiting for
    commands from the service controller.

    \sa QtServiceController
*/
int QtServiceBase::exec()
{
    if (d_ptr->args.size() > 1) {
        QString a =  d_ptr->args.at(1);
        if (a == QLatin1String("-i") || a == QLatin1String("-install")) {
            if (!d_ptr->controller.isInstalled()) {
                QString account;
                QString password;
                if (d_ptr->args.size() > 2)
                    account = d_ptr->args.at(2);
                if (d_ptr->args.size() > 3)
                    password = d_ptr->args.at(3);
                if (!d_ptr->install(account, password)) {
                    fprintf(stderr, "The service %s could not be installed\n", serviceName().toLatin1().constData());
                    return -1;
                } else {
                    printf("The service %s has been installed under: %s\n",
                        serviceName().toLatin1().constData(), d_ptr->filePath().toLatin1().constData());
                }
            } else {
                fprintf(stderr, "The service %s is already installed\n", serviceName().toLatin1().constData());
            }
            return 0;
        } else if (a == QLatin1String("-u") || a == QLatin1String("-uninstall")) {
            if (d_ptr->controller.isInstalled()) {
                if (!d_ptr->controller.uninstall()) {
                    fprintf(stderr, "The service %s could not be uninstalled\n", serviceName().toLatin1().constData());
                    return -1;
                } else {
                    printf("The service %s has been uninstalled.\n",
                        serviceName().toLatin1().constData());
                }
            } else {
                fprintf(stderr, "The service %s is not installed\n", serviceName().toLatin1().constData());
            }
            return 0;
        } else if (a == QLatin1String("-v") || a == QLatin1String("-version")) {
            printf("The service\n"
                "\t%s\n\t%s\n\n", serviceName().toLatin1().constData(), d_ptr->args.at(0).toLatin1().constData());
            printf("is %s", (d_ptr->controller.isInstalled() ? "installed" : "not installed"));
            printf(" and %s\n\n", (d_ptr->controller.isRunning() ? "running" : "not running"));
            return 0;
        } else if (a == QLatin1String("-e") || a == QLatin1String("-exec")) {
            d_ptr->args.removeAt(1);
            int ec = d_ptr->run(false, d_ptr->args);
            if (ec == -1)
                qErrnoWarning("The service could not be executed.");
            return ec;
        } else if (a == QLatin1String("-t") || a == QLatin1String("-terminate")) {
            if (!d_ptr->controller.stop())
                qErrnoWarning("The service could not be stopped.");
            return 0;
        } else if (a == QLatin1String("-p") || a == QLatin1String("-pause")) {
            d_ptr->controller.pause();
            return 0;
        } else if (a == QLatin1String("-r") || a == QLatin1String("-resume")) {
            d_ptr->controller.resume();
            return 0;
        } else if (a == QLatin1String("-c") || a == QLatin1String("-command")) {
            int code = 0;
            if (d_ptr->args.size() > 2)
                code = d_ptr->args.at(2).toInt();
            d_ptr->controller.sendCommand(code);
            return 0;
        } else  if (a == QLatin1String("-h") || a == QLatin1String("-help")) {
            printf("\n%s -[i|u|e|t|p|r|c|v|h]\n"
                   "\t-i(nstall) [account] [password]\t: Install the service, optionally using given account and password\n"
                   "\t-u(ninstall)\t: Uninstall the service.\n"
                   "\t-e(xec)\t\t: Run as a regular application. Useful for debugging.\n"
                   "\t-t(erminate)\t: Stop the service.\n"
                   "\t-p(ause)\t: Pause the service.\n"
                   "\t-r(esume)\t: Resume a paused service.\n"
                   "\t-c(ommand) num\t: Send command code num to the service.\n"
                   "\t-v(ersion)\t: Print version and status information.\n"
                   "\t-h(elp)   \t: Show this help\n"
                   "\tNo arguments\t: Start the service.\n",
                   d_ptr->args.at(0).toLatin1().constData());
            return 0;
        }
    }
#if defined(Q_OS_UNIX)
    if (::getenv("QTSERVICE_RUN")) {
        // Means we're the detached, real service process.
        int ec = d_ptr->run(true, d_ptr->args);
        if (ec == -1)
            qErrnoWarning("The service failed to run.");
        return ec;
    }
#endif
    if (!d_ptr->start()) {
        fprintf(stderr, "The service %s could not start\n", serviceName().toLatin1().constData());
        return -4;
    }
    return 0;
}

/*!
    \fn void QtServiceBase::logMessage(const QString &message, MessageType type,
            int id, uint category, const QByteArray &data)

    Reports a message of the given \a type with the given \a message
    to the local system event log.  The message identifier \a id and
    the message \a category are user defined values. The \a data
    parameter can contain arbitrary binary data.

    Message strings for \a id and \a category must be provided by a
    message file, which must be registered in the system registry.
    Refer to the MSDN for more information about how to do this on
    Windows.

    \sa MessageType
*/

/*!
    Returns a pointer to the current application's QtServiceBase
    instance.
*/
QtServiceBase *QtServiceBase::instance()
{
    return QtServiceBasePrivate::instance;
}

/*!
    \fn void QtServiceBase::start()

    This function must be implemented in QtServiceBase subclasses in
    order to perform the service's work. Usually you create some main
    object on the heap which is the heart of your service.

    The function is only called when no service specific arguments
    were passed to the service constructor, and is called by exec()
    after it has called the executeApplication() function.

    Note that you \e don't need to create an application object or
    call its exec() function explicitly.

    \sa exec(), stop(), QtServiceController::start()
*/

/*!
    Reimplement this function to perform additional cleanups before
    shutting down (for example deleting a main object if it was
    created in the start() function).

    This function is called in reply to controller requests. The
    default implementation does nothing.

    \sa start(), QtServiceController::stop()
*/
void QtServiceBase::stop()
{
}

/*!
    Reimplement this function to pause the service's execution (for
    example to stop a polling timer, or to ignore socket notifiers).

    This function is called in reply to controller requests.  The
    default implementation does nothing.

    \sa resume(), QtServiceController::pause()
*/
void QtServiceBase::pause()
{
}

/*!
    Reimplement this function to continue the service after a call to
    pause().

    This function is called in reply to controller requests. The
    default implementation does nothing.

    \sa pause(), QtServiceController::resume()
*/
void QtServiceBase::resume()
{
}

/*!
    Reimplement this function to process the user command \a code.


    This function is called in reply to controller requests.  The
    default implementation does nothing.

    \sa QtServiceController::sendCommand()
*/
void QtServiceBase::processCommand(int /*code*/)
{
}

/*!
    \fn void QtServiceBase::createApplication(int &argc, char **argv)

    Creates the application object using the \a argc and \a argv
    parameters.

    This function is only called when no \l
    {serviceSpecificArguments}{service specific arguments} were
    passed to the service constructor, and is called by exec() before
    it calls the executeApplication() and start() functions.

    The createApplication() function is implemented in QtService, but
    you might want to reimplement it, for example, if the chosen
    application type's constructor needs additional arguments.

    \sa exec(), QtService
*/

/*!
    \fn int QtServiceBase::executeApplication()

    Executes the application previously created with the
    createApplication() function.

    This function is only called when no \l
    {serviceSpecificArguments}{service specific arguments} were
    passed to the service constructor, and is called by exec() after
    it has called the createApplication() function and before start() function.

    This function is implemented in QtService.

    \sa exec(), createApplication()
*/

/*!
    \class QtService

    \brief The QtService is a convenient template class that allows
    you to create a service for a particular application type.

    A Windows service or Unix daemon (a "service"), is a program that
    runs "in the background" independently of whether a user is logged
    in or not. A service is often set up to start when the machine
    boots up, and will typically run continuously as long as the
    machine is on.

    Services are usually non-interactive console applications. User
    interaction, if required, is usually implemented in a separate,
    normal GUI application that communicates with the service through
    an IPC channel. For simple communication,
    QtServiceController::sendCommand() and QtService::processCommand()
    may be used, possibly in combination with a shared settings file. For
    more complex, interactive communication, a custom IPC channel
    should be used, e.g. based on Qt's networking classes. (In certain
    circumstances, a service may provide a GUI itself, ref. the
    "interactive" example documentation).

    \bold{Note:} On Unix systems, this class relies on facilities
    provided by the QtNetwork module, provided as part of the
    \l{Qt Open Source Edition} and certain \l{Qt Commercial Editions}.

    The QtService class functionality is inherited from QtServiceBase,
    but in addition the QtService class binds an instance of
    QtServiceBase with an application type.

    Typically, you will create a service by subclassing the QtService
    template class. For example:

    \code
    class MyService : public QtService<QApplication>
    {
    public:
        MyService(int argc, char **argv);
        ~MyService();

    protected:
        void start();
        void stop();
        void pause();
        void resume();
        void processCommand(int code);
    };
    \endcode

    The application type can be QCoreApplication for services without
    GUI, QApplication for services with GUI or you can use your own
    custom application type.

    You must reimplement the QtServiceBase::start() function to
    perform the service's work. Usually you create some main object on
    the heap which is the heart of your service.

    In addition, you might want to reimplement the
    QtServiceBase::pause(), QtServiceBase::processCommand(),
    QtServiceBase::resume() and QtServiceBase::stop() to intervene the
    service's process on controller requests. You can control any
    given service using an instance of the QtServiceController class
    which also allows you to control services from separate
    applications. The mentioned functions are all virtual and won't do
    anything unless they are reimplemented.

    Your custom service is typically instantiated in the application's
    main function. Then the main function will call your service's
    exec() function, and return the result of that call. For example:

    \code
        int main(int argc, char **argv)
        {
            MyService service(argc, argv);
            return service.exec();
        }
    \endcode

    When the exec() function is called, it will parse the \l
    {serviceSpecificArguments} {service specific arguments} passed in
    \c argv, perform the required actions, and exit.

    If none of the arguments is recognized as service specific, exec()
    will first call the createApplication() function, then executeApplication() and
    finally the start() function. In the end, exec()
    returns while the service continues in its own process waiting for
    commands from the service controller.

    \sa QtServiceBase, QtServiceController
*/

/*!
    \fn QtService::QtService(int argc, char **argv, const QString &name)

    Constructs a QtService object called \a name. The \a argc and \a
    argv parameters are parsed after the exec() function has been
    called. Then they are passed to the application's constructor.

    There can only be one QtService object in a process.

    \sa QtServiceBase()
*/

/*!
    \fn QtService::~QtService()

    Destroys the service object.
*/

/*!
    \fn Application *QtService::application() const

    Returns a pointer to the application object.
*/

/*!
    \fn void QtService::createApplication(int &argc, char **argv)

    Creates application object of type Application passing \a argc and
    \a argv to its constructor.

    \reimp

*/

/*!
    \fn int QtService::executeApplication()

    \reimp
*/
