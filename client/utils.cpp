#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include "defines.h"
#include "utils.h"

QString Utils::toString(bool value)
{
    return value ? "true" : "false";
}

QString Utils::serverName()
{
#ifdef Q_OS_WIN
    return SERVICE_NAME;
#else
    return QString("/tmp/%1").arg(SERVICE_NAME);
#endif
}

QString Utils::defaultVpnConfigFileName()
{
    return configPath() + QString("/%1.ovpn").arg(APPLICATION_NAME);
}

QString Utils::systemLogPath()
{
#ifdef Q_OS_WIN
    QStringList locationList = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString primaryLocation = "ProgramData";
    foreach (const QString& location, locationList) {
        if (location.contains(primaryLocation)) {
            return QString("%1/%2/log").arg(location).arg(APPLICATION_NAME);
        }
    }
    return QString();
#else
    return QString("/var/log/%1").arg(APPLICATION_NAME);
#endif
}

bool Utils::initializePath(const QString& path)
{
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning().noquote() << QString("Cannot initialize path: '%1'").arg(path);
        return false;
    }
    return true;
}

QString Utils::configPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/config";
}

bool Utils::createEmptyFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::executable(const QString& baseName, bool absPath)
{
    QString ext;
#ifdef Q_OS_WIN
    ext = ".exe";
#endif
    const QString fileName = baseName + ext;
    if (!absPath) {
        return fileName;
    }
    return QCoreApplication::applicationDirPath() + "/" + fileName;
}

bool Utils::processIsRunning(const QString& fileName)
{
#ifdef Q_OS_WIN
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(QString("wmic.exe /OUTPUT:STDOUT PROCESS get %1").arg("Caption"));
    process.waitForStarted();
    process.waitForFinished();
    QString processData(process.readAll());
    QStringList processList = processData.split(QRegExp("[\r\n]"),QString::SkipEmptyParts);
    foreach (const QString& rawLine, processList) {
        const QString line = rawLine.simplified();
        if (line.isEmpty()) {
            continue;
        }

        if (line == fileName) {
            return true;
        }

    }
    return false;
#else
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("pgrep", QStringList({fileName}));
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit) {
        return (process.readAll().toUInt() > 0);
    }
    return false;
#endif
}

QString Utils::getIPAddress(const QString& host)
{
    //TODO rewrite to api calls
    qDebug().noquote() << "GetIPAddress: checking " + host;
    if(host.isEmpty()) {
        qDebug().noquote() << "GetIPAddress: host is empty.";
        return QString();
    }

    if(checkIPFormat(host)) {
        qDebug().noquote() << "GetIPAddress host is ip:" << host << host;
        return host;    // it is a ip address.
    }
    QProcess ping;

#ifdef Q_OS_MACX
    ping.start("ping", QStringList() << "-c1" << host);
#endif
#ifdef Q_OS_WIN
    ping.start("ping", QStringList() << QString("/n") << "1" << QString("/w") << "1" << host);
#endif
    ping.waitForStarted();

    QEventLoop loop;
    loop.connect(&ping, SIGNAL(finished(int)), &loop, SLOT(quit()));
    loop.exec();

    QString d = ping.readAll();
    if(d.size() == 0)
        return QString();
    qDebug().noquote() << d;

    QString ip;
#ifdef Q_OS_MACX
    ip = getStringBetween(d, "(", ")");
#endif
#ifdef Q_OS_WIN
    ip = getStringBetween(d, "[", "]");
#endif
    qDebug().noquote() << "GetIPAddress:" << host << ip;
    return ip;
}

QString Utils::getStringBetween(const QString& s, const QString& a, const QString& b)
{
    int ap = s.indexOf(a), bp = s.indexOf(b, ap + a.length());
    if(ap < 0 || bp < 0)
        return QString();
    ap += a.length();
    if(bp - ap <= 0)
        return QString();
    return s.mid(ap, bp - ap).trimmed();
}

bool Utils::checkIPFormat(const QString& ip)
{
    int count = ip.count(".");
    if(count != 3)
        return false;

    QStringList list = ip.trimmed().split(".");
    foreach(QString it, list) {
        if(it.toInt() <= 255 && it.toInt() >= 0)
            continue;
        return false;
    }
    return true;
}
