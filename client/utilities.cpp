#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QProcess>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>

#include "utilities.h"
#include "version.h"

QString Utils::getRandomString(int len)
{
    const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789");

    QString randomString;
    for (int i = 0; i < len; ++i) {
        quint32 index = QRandomGenerator::global()->generate() % possibleCharacters.length();
        QChar nextChar = possibleCharacters.at(index);
        randomString.append(nextChar);
    }
    return randomString;
}

QString Utils::VerifyJsonString(const QString &source)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(source.toUtf8(), &error);
    Q_UNUSED(doc)

    if (error.error == QJsonParseError::NoError) {
        return "";
    } else {
        qDebug() << "WARNING: Json parse returns: " + error.errorString();
        return error.errorString();
    }
}

QJsonObject Utils::JsonFromString(const QString &string)
{
    auto removeComment = string.trimmed();
    if (removeComment != string.trimmed()) {
        qDebug() << "Some comments have been removed from the json.";
    }
    QJsonDocument doc = QJsonDocument::fromJson(removeComment.toUtf8());
    return doc.object();
}

QString Utils::SafeBase64Decode(QString string)
{
    QByteArray ba = string.replace(QChar('-'), QChar('+')).replace(QChar('_'), QChar('/')).toUtf8();
    return QByteArray::fromBase64(ba, QByteArray::Base64Option::OmitTrailingEquals);
}

QString Utils::JsonToString(const QJsonObject &json, QJsonDocument::JsonFormat format)
{
    QJsonDocument doc;
    doc.setObject(json);
    return doc.toJson(format);
}

QString Utils::JsonToString(const QJsonArray &array, QJsonDocument::JsonFormat format)
{
    QJsonDocument doc;
    doc.setArray(array);
    return doc.toJson(format);
}

bool Utils::initializePath(const QString &path)
{
    QDir dir;
    if (!dir.mkpath(path)) {
        qWarning().noquote() << QString("Cannot initialize path: '%1'").arg(path);
        return false;
    }
    return true;
}

bool Utils::createEmptyFile(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::WriteOnly | QIODevice::Truncate);
}

QString Utils::executable(const QString &baseName, bool absPath)
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

QString Utils::usrExecutable(const QString &baseName)
{
    if (QFileInfo::exists("/usr/sbin/" + baseName))
        return ("/usr/sbin/" + baseName);
    else
        return ("/usr/bin/" + baseName);
}

bool Utils::processIsRunning(const QString &fileName, const bool fullFlag)
{
#ifdef Q_OS_WIN
    QProcess process;
    process.setReadChannel(QProcess::StandardOutput);
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("wmic.exe",
                  QStringList() << "/OUTPUT:STDOUT"
                                << "PROCESS"
                                << "get"
                                << "Caption");
    process.waitForStarted();
    process.waitForFinished();
    QString processData(process.readAll());
    QStringList processList = processData.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    foreach (const QString &rawLine, processList) {
        const QString line = rawLine.simplified();
        if (line.isEmpty()) {
            continue;
        }

        if (line == fileName) {
            return true;
        }
    }
    return false;
#elif defined(Q_OS_IOS)
    return false;
#else
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start("pgrep", QStringList({ fullFlag ? "-f" : "", fileName }));
    process.waitForFinished();
    if (process.exitStatus() == QProcess::NormalExit) {
        if (fullFlag) {
            return (process.readLine().toUInt() > 0);
        } else {
            return (process.readAll().toUInt() > 0);
        }
    }
    return false;
#endif
}

void Utils::killProcessByName(const QString &name)
{
    qDebug().noquote() << "Kill process" << name;
#ifdef Q_OS_WIN
    QProcess::execute("taskkill", QStringList() << "/IM" << name << "/F");
#elif defined Q_OS_IOS
    return;
#else
    QProcess::execute(QString("pkill %1").arg(name));
#endif
}

QString Utils::openVpnExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("openvpn/openvpn", true);
#elif defined Q_OS_LINUX
    // We have service that runs OpenVPN on Linux. We need to make same
    // path for client and service.
    return Utils::executable("../../client/bin/openvpn", true);
#else
    return Utils::executable("/openvpn", true);
#endif
}

QString Utils::wireguardExecPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("wireguard/wireguard-service", true);
#elif defined Q_OS_LINUX
    return Utils::usrExecutable("wg-quick");
#elif defined Q_OS_MAC
    return Utils::executable("/wireguard", true);
#else
    return {};
#endif
}

QString Utils::certUtilPath()
{
#ifdef Q_OS_WIN
    QString winPath = QString::fromUtf8(qgetenv("windir"));
    return winPath + "\\system32\\certutil.exe";
#else
    return "";
#endif
}

QString Utils::tun2socksPath()
{
#ifdef Q_OS_WIN
    return Utils::executable("xray/tun2socks", true);
#elif defined Q_OS_LINUX
    // We have service that runs OpenVPN on Linux. We need to make same
    // path for client and service.
    return Utils::executable("../../client/bin/tun2socks", true);
#else
    return Utils::executable("/tun2socks", true);
#endif
}

#ifdef Q_OS_WIN
// Inspired from http://stackoverflow.com/a/15281070/1529139
// and http://stackoverflow.com/q/40059902/1529139
bool Utils::signalCtrl(DWORD dwProcessId, DWORD dwCtrlEvent)
{
    bool success = false;
    DWORD thisConsoleId = GetCurrentProcessId();
    // Leave current console if it exists
    // (otherwise AttachConsole will return ERROR_ACCESS_DENIED)
    bool consoleDetached = (FreeConsole() != FALSE);

    if (AttachConsole(dwProcessId) != FALSE) {
        // Add a fake Ctrl-C handler for avoid instant kill is this console
        // WARNING: do not revert it or current program will be also killed
        SetConsoleCtrlHandler(nullptr, true);
        success = (GenerateConsoleCtrlEvent(dwCtrlEvent, 0) != FALSE);
        FreeConsole();
    }

    if (consoleDetached) {
        // Create a new console if previous was deleted by OS
        if (AttachConsole(thisConsoleId) == FALSE) {
            int errorCode = GetLastError();
            if (errorCode == 31) // 31=ERROR_GEN_FAILURE
            {
                AllocConsole();
            }
        }
    }
    return success;
}

#endif

void Utils::logException(const std::exception &e)
{
    qCritical() << e.what();
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception &nested) {
        logException(nested);
    } catch (...) {}
}

void Utils::logException(const std::exception_ptr &eptr)
{
    try {
        if (eptr) std::rethrow_exception(eptr);
    } catch (const std::exception &e) {
        logException(e);
    } catch (...) {}
}
