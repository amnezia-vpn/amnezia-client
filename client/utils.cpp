#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>

#include "defines.h"
#include "utils.h"

QString Utils::toString(bool value)
{
    return value ? "true" : "false";
}

QString Utils::systemLogPath()
{
    return systemDataLocationPath() + "/log";
}

QString Utils::systemConfigPath()
{
    return systemDataLocationPath() + "/config";
}

bool Utils::createEmptyFile(const QString& path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::systemDataLocationPath()
{
    QStringList locationList = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    QString primaryLocation;
#ifdef Q_OS_WIN
    primaryLocation = "ProgramData";
#elif defined Q_OS_MAC
    primaryLocation = "Users";
#endif

    foreach (const QString& location, locationList) {
        if (location.contains(primaryLocation)) {
            return QString("%1/%2").arg(location).arg(APPLICATION_NAME);
        }
    }

    return QString();
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
    process.setReadChannelMode(QProcess::MergedChannels);
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

