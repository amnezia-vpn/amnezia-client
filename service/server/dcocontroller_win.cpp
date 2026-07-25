#include <QCoreApplication>
#include <QDebug>
#include <QOperatingSystemVersion>
#include <QProcess>

#include "dcocontroller_win.h"

DcoController &DcoController::Instance()
{
    static DcoController s;
    return s;
}

QString DcoController::getDcoDriverDir()
{
    // win11 flavour is NetAdapterCx 2.1, win10 (2004+) flavour is 2.0
    const QString flavour =
            QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows11 ? "win11" : "win10";
    return qApp->applicationDirPath() + "\\dco\\" + flavour;
}

QString DcoController::getTapctlPath()
{
    return qApp->applicationDirPath() + "\\tapctl.exe";
}

bool DcoController::isDriverInstalled()
{
    QProcess proc;
    proc.start("pnputil", { "/enum-drivers" });
    if (!proc.waitForStarted()) {
        qWarning() << "DcoController: failed to start pnputil";
        return false;
    }
    proc.waitForFinished();

    // the driver store lists packages by their original inf name
    return QString(proc.readAllStandardOutput()).contains("ovpn-dco.inf", Qt::CaseInsensitive);
}

bool DcoController::installDriver()
{
    const QString inf = getDcoDriverDir() + "\\ovpn-dco.inf";

    QProcess proc;
    proc.start("pnputil", { "/add-driver", inf, "/install" });
    if (!proc.waitForStarted()) {
        qWarning() << "DcoController: failed to start pnputil";
        return false;
    }
    proc.waitForFinished(60000);

    const QString output = QString(proc.readAllStandardOutput());
    qDebug().noquote() << "DcoController: pnputil /add-driver" << inf << "exit code"
                       << proc.exitCode() << "output:" << output;

    // 0 = ok, 3010 = ok but reboot required
    return proc.exitCode() == 0 || proc.exitCode() == 3010;
}

bool DcoController::adapterExists()
{
    QProcess proc;
    proc.start(getTapctlPath(), { "list" });
    if (!proc.waitForStarted()) {
        qWarning() << "DcoController: failed to start tapctl.exe";
        return false;
    }
    proc.waitForFinished();

    return QString(proc.readAllStandardOutput()).contains(kAdapterName);
}

bool DcoController::createAdapter()
{
    QProcess proc;
    proc.start(getTapctlPath(), { "create", "--hwid", "ovpn-dco", "--name", kAdapterName });
    if (!proc.waitForStarted()) {
        qWarning() << "DcoController: failed to start tapctl.exe";
        return false;
    }
    proc.waitForFinished(60000);

    const QString output = QString(proc.readAllStandardOutput()) + QString(proc.readAllStandardError());
    qDebug().noquote() << "DcoController: tapctl create exit code" << proc.exitCode()
                       << "output:" << output;

    return proc.exitCode() == 0;
}

bool DcoController::checkAndSetup()
{
    qDebug().noquote() << "DcoController: driver dir" << getDcoDriverDir();

    if (!isDriverInstalled()) {
        qDebug() << "DcoController: ovpn-dco driver not found, installing...";
        if (!installDriver()) {
            qWarning() << "DcoController: driver installation failed";
            return false;
        }
    }

    if (!adapterExists()) {
        qDebug() << "DcoController: creating" << kAdapterName << "adapter...";
        if (!createAdapter()) {
            qWarning() << "DcoController: adapter creation failed";
            return false;
        }
    }

    qDebug() << "DcoController: ovpn-dco driver and adapter are ready";
    return true;
}
